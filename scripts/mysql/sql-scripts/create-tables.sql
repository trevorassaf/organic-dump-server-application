DROP DATABASE plantsandthings;
CREATE DATABASE plantsandthings;
USE plantsandthings;

CREATE TABLE rpis (
  id INT AUTO_INCREMENT NOT NULL,
  PRIMARY KEY(id),
  name VARCHAR(256) NOT NULL,
  time VARCHAR(32) NOT NULL,
  location VARCHAR(256) NOT NULL);

CREATE TABLE peripherals (
  id INT NOT NULL AUTO_INCREMENT,
  PRIMARY KEY(id),
  name VARCHAR(64) NOT NULL UNIQUE,
  time VARCHAR(32) NOT NULL);

CREATE TABLE rpi_peripheral_edges (
  rpi_id INT NOT NULL,
  FOREIGN KEY(rpi_id) REFERENCES rpis(id),
  peripheral_id INT NOT NULL,
  FOREIGN KEY(peripheral_id) REFERENCES peripherals(id),
  PRIMARY KEY(peripheral_id));

CREATE TABLE soil_moisture_sensors (
  peripheral_id INT NOT NULL,
  FOREIGN KEY(peripheral_id) REFERENCES peripherals(id),
  PRIMARY KEY(peripheral_id),
  ceiling FLOAT NOT NULL,
  floor FLOAT NOT NULL);

CREATE TABLE soil_moisture_readings (
  id INT AUTO_INCREMENT,
  PRIMARY KEY(id),
  time VARCHAR(32) NOT NULL,
  reading FLOAT NOT NULL,
  sensor_id INT NOT NULL,
  FOREIGN KEY(sensor_id) REFERENCES soil_moisture_sensors(peripheral_id));

CREATE TABLE irrigation_systems (
  peripheral_id INT NOT NULL,
  FOREIGN KEY(peripheral_id) REFERENCES peripherals(id),
  PRIMARY KEY(peripheral_id));

CREATE TABLE daily_irrigation_schedules (
  id INT AUTO_INCREMENT,
  PRIMARY KEY(id),
  day_of_week_index INT NOT NULL,
  irrigation_time_military VARCHAR(32) NOT NULL,
  duration_ms INT NOT NULL,
  irrigation_system_id INT NOT NULL,
  FOREIGN KEY(irrigation_system_id) REFERENCES irrigation_systems(peripheral_id));

SHOW TABLES;
