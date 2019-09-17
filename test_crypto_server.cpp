#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

DEFINE_int32(port, -1, "Port");
DEFINE_string(cert, "", "Certificate file");
DEFINE_string(key, "", "Private key file");
DEFINE_string(ca, "", "CA file");
DEFINE_string(message, "Default client message", "Message to send client");

namespace
{
constexpr size_t CONNECTION_QUEUE_SIZE = 10;
constexpr size_t MESSAGE_BUFFER_SIZE = 100;

void PrintPeerCert(SSL *ssl)
{
    assert(ssl);

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert == nullptr)
    {
        LOG(INFO) << "Peer certificate: none";
        return;
    }

    std::unique_ptr<char[]> line;

    LOG(ERROR) << "Peer certificate:";
    line.reset(X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
    LOG(ERROR) << "- Subject: " << line.get();
    line.reset(X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0));
    LOG(ERROR) << "- Issuer: " << line.get();
    X509_free(cert);
    return;
}

bool DoPeerConversation(SSL *ssl, const char *message)
{
    LOG(INFO) << "Waiting for client message...";

    char clientMessageBuffer[MESSAGE_BUFFER_SIZE];
    memset(clientMessageBuffer, 0, sizeof(clientMessageBuffer));

    int readResult = SSL_read(ssl, clientMessageBuffer, sizeof(clientMessageBuffer));
    if (readResult < 1)
    {
        ERR_print_errors_fp(stdout);
        LOG(ERROR) << "Failed to read client message. SSL error: " << SSL_get_error(ssl, readResult);
        return false;
    }

    LOG(INFO) << "Received client message: " << clientMessageBuffer;
    LOG(INFO) << "Sending message to client...";

    int writeResult = SSL_write(ssl, message, strlen(message));
    if (writeResult < 1)
    {
        LOG(ERROR) << "Failed to write client message";
        return false;
    }

    return true;
}

bool RunServer(SSL_CTX *ctx, int serverFd, const char *message)
{
    assert(ctx);

    LOG(INFO) << "Starting run server";

    while (true)
    {
        struct sockaddr_in connectionAddr;
        socklen_t connectionAddrLen = sizeof(connectionAddr);
        SSL *ssl = nullptr;
        bool isError = true;

        int connectionFd = accept(
            serverFd,
            reinterpret_cast<struct sockaddr *>(&connectionAddr),
            &connectionAddrLen);

        if (connectionFd < 0)
        {
            LOG(ERROR) << "Failed to accept connection";
            goto finished;
        }

        char peerIpv4String[INET_ADDRSTRLEN];
        LOG(ERROR) << "Accepted connection from "
                   << inet_ntop(AF_INET, &(connectionAddr.sin_addr), peerIpv4String, INET_ADDRSTRLEN);

        ssl = SSL_new(ctx);

        if (ssl == nullptr)
        {
            LOG(ERROR) << "Failed to initialize SSL connection";
            goto finished;
        }

        SSL_set_fd(ssl, connectionFd);

        if (SSL_accept(ssl) <= 0)
        {
            LOG(ERROR) << "Failed to conduct SSL handshake";
            ERR_print_errors_fp(stderr);
            goto finished;
        }

        LOG(INFO) << "Server succeeded in accepting connection";

        /*
        PrintPeerCert(ssl);

        LOG(INFO) << "Server succeeded in printing peer cert";
        */

        if (!DoPeerConversation(ssl, message))
        {
            LOG(ERROR) << "Failed to conduct conversation with peer";
            goto finished;
        }

        LOG(INFO) << "Server succeeded in performing peer conversation";

        LOG(INFO) << "Finished conversation with client";
        goto finished;

finished:
        SSL_free(ssl);
        close(connectionFd);
    }

    return true;
}

bool OpenListeningSocket(SSL_CTX *ctx, uint16_t port, uint32_t *outServerFd)
{
    assert(ctx);
    assert(outServerFd);


    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        LOG(ERROR) << "Failed to initialize socket";
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        LOG(ERROR) << "Failed to bind socket";
        close(fd);
        return false;
    }

    if (listen(fd, CONNECTION_QUEUE_SIZE) < 0)
    {
        LOG(ERROR) << "Failed to configure socket in listening mode";
        close(fd);
        return false;
    }

    *outServerFd = fd;
    return true;
}

bool LoadCertificate(SSL_CTX *ctx, const char *certFile, const char *keyFile, const char *caFile)
{
    assert(ctx);
    assert(certFile);
    assert(keyFile);

    // Load CA cert
    if (SSL_CTX_load_verify_locations(ctx, caFile, nullptr) != 1)
    {
        LOG(ERROR) << "Failed to load verify locations for ca cert: " << caFile;
        return false;
    }

    SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(caFile));

    // Load server cert
    if (SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) != 1)
    {
        LOG(ERROR) << "Failed to use certificate file: " << certFile;
        return false;
    }

    // Load server private key
    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) != 1)
    {
        LOG(ERROR) << "Failed to load private key: " << keyFile;
        return false;
    }

    // Ensure cert and private key agree with one another
    if (SSL_CTX_check_private_key(ctx) != 1)
    {
        LOG(ERROR) << "Private key does not match public key";
        return false;
    }

    // Require mutual authentication. Only accept peer certs signed by CA directly.
    SSL_CTX_set_verify(
        ctx,
        SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        nullptr);

    // Accept only certs signed by CA itself. No cert chaining.
    SSL_CTX_set_verify_depth(ctx, 1);

    return true;
}

bool InitSslCtx(SSL_CTX **outCtx)
{
    assert(outCtx);

    const SSL_METHOD *method = TLSv1_2_server_method();
    *outCtx = SSL_CTX_new(method);

    if (*outCtx == nullptr)
    {
        LOG(ERROR) << "Failed to initialize ctx";
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

void RequireMutualAuth(SSL_CTX *ctx, const char *ca_file)
{
  /*
    assert(ctx);

    // Require mutual authentication. Only accept peer certs signed by CA directly.
    SSL_CTX_set_verify(
        ctx,
        SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        nullptr);

    // Load CA for client cert. We will verify that the client's cert is signed by this CA.
    SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(ca_file));

    // Accept only certs signed by CA itself. No cert chaining.
    SSL_CTX_set_verify_depth(ctx, 1);

    */
}
} // namespace

int main(int argc, char **argv)
{
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  SSL_CTX *ctx;
  uint32_t serverFd;

  if (!InitSslCtx(&ctx))
  {
      LOG(ERROR) << "Failed to initialize SSL context";
      goto error;
  }

  if (!LoadCertificate(ctx, FLAGS_cert.c_str(), FLAGS_key.c_str(), FLAGS_ca.c_str()))
  {
      LOG(ERROR) << "Failed to load cert";
      goto error;
  }

  RequireMutualAuth(ctx, FLAGS_ca.c_str());

  if (!OpenListeningSocket(ctx, static_cast<uint16_t>(FLAGS_port), &serverFd))
  {
      LOG(ERROR) << "Failed to open listening socket";
      goto error;
  }

  if (!RunServer(ctx, serverFd, FLAGS_message.c_str()))
  {
      LOG(ERROR) << "Failed to run server";
      goto error;
  }

  return EXIT_SUCCESS;

error:
  SSL_CTX_free(ctx);
  return EXIT_FAILURE;
}
