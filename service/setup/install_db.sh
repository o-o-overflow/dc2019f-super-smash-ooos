#!/bin/bash

# Initialize MySQL database.
# ADD this file into the container via Dockerfile.
# Assuming you specify a VOLUME ["/var/lib/mysql"] or `-v /var/lib/mysql` on the `docker run` command…
# Once built, do e.g. `docker run your_image /path/to/docker-mysql-initialize.sh`
# Again, make sure MySQL is persisting data outside the container for this to have any effect.

set -e
set -x

#/usr/sbin/mysqld --initialize

# Start the MySQL daemon in the background.
/etc/init.d/mysql start
mysql_pid=$!

until mysqladmin ping >/dev/null 2>&1; do
  echo -n "."; sleep 0.2
done

# Permit root login without password from outside container.
#mysql -e "GRANT ALL ON *.* TO root@'%' IDENTIFIED BY '' WITH GRANT OPTION"

# create the default database from the ADDed file.
mysql < /root/db.sql

mysql ccdb -e "select count(*) from accounts"

# Tell the MySQL daemon to shutdown.
/etc/init.d/mysql stop

# Wait for the MySQL daemon to exit.
wait $mysql_pid

