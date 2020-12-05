#!/bin/bash

# Set up project structure
mkdir -p codez/plantsandthings/organic_dump_server
mkdir codez/tools


### Install deps

# Linux Utilities
sudo apt-get install wget

# MYSQL 8
mkdir codez/tools/mysql-8
pushd codez/tools/mysql-8

#wget https://dev.mysql.com/get/mysql-apt-config_0.8.16-1_all.deb
#sudo dpkg -i mysql-apt-config_0.8.16-1_all.deb

sudo apt-get install mysql-server
