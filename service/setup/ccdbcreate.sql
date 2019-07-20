
CREATE DATABASE IF NOT EXISTS `ccdb`;

CREATE USER 'ccuser'@'%' IDENTIFIED BY 'ccuser';

GRANT USAGE ON * . * TO 'ccuser'@'%' IDENTIFIED BY 'ccuser' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0 MAX_USER_CONNECTIONS 0 ;

GRANT SELECT ON `ccdb` . * TO 'ccuser'@'%';
GRANT INSERT ON `ccdb` . * TO 'ccuser'@'%';

USE `ccdb`;

CREATE TABLE accounts ( pan VARCHAR(20) CHARACTER SET utf8, balance decimal(13, 2) not null default 0, credit_limit decimal(13, 2) not null default 200, PRIMARY KEY (pan) );

CREATE TABLE permitted ( id INT NOT NULL AUTO_INCREMENT, valid_referer TEXT CHARACTER SET utf8, PRIMARY KEY (id) );

INSERT INTO `accounts` VALUES("63603378901234566",2000, 10000);
INSERT INTO `accounts` VALUES("6360337890123455",2000, 10000);

INSERT INTO `permitted` (valid_referer) VALUES ('secret_referer');
INSERT INTO `permitted` (valid_referer) VALUES ('/purchase.html');

CREATE TABLE approval ( approval_code char(6) CHARACTER SET utf8 NOT NULL, fk_pan VARCHAR(20) CHARACTER SET utf8, PRIMARY KEY (approval_code) );




