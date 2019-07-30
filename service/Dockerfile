FROM node:8

ENV TERM linux
## for apt to be noninteractive
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true
RUN echo "travis_fold:start:Dapt\033[33;1mservice Dockerfile apt\033[0m" && \
    apt update && \
    apt install -y apache2 software-properties-common mysql-server wget emacs supervisor \
                   # python 3
                   python3-pip python3-pexpect \
                   # for mysqlclient
                   python3-dev default-libmysqlclient-dev && \
    echo "\ntravis_fold:end:Dapt\r"

RUN echo "travis_fold:start:DIpip\033[33;1mservice Dockerfile pip\033[0m" && \
    pip3 install websockets termcolor mysqlclient && \
    echo "\ntravis_fold:end:DIpip\r"

COPY setup/000-default.conf /etc/apache2/sites-available/000-default.conf
COPY setup/envvars /etc/apache2/envvars
COPY setup/serve-cgi-bin.conf /etc/apache2/conf-available/serve-cgi-bin.conf
COPY setup/ports.conf /etc/apache2/ports.conf
COPY setup/apache2.conf /etc/apache2/apache2.conf
RUN ln -s /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enabled/cgi.load


# mysql setup and populate
COPY setup/mysqld.cnf /etc/mysql/mysql.conf.d/mysqld.cnf
COPY setup/install_db.sh /root/install_db.sh
RUN chmod 0700 /root/install_db.sh && chown root:root /root/install_db.sh
COPY setup/ccdbcreate.sql /root/db.sql
RUN chmod 0400 /root/db.sql && chown root:root /root/db.sql

COPY setup/supervisord.conf /etc/supervisord.conf

RUN cd /root && /root/install_db.sh

RUN useradd -s /bin/bash -m oooser
RUN mkdir /tmp/nodesessions && chown oooser:www-data /tmp/nodesessions && chmod 770 /tmp/nodesessions

WORKDIR /app

RUN npm install ws

RUN touch /var/log/websocket.log && chmod 0600 /var/log/websocket.log && chown oooser:oooser /var/log/websocket.log
RUN echo "OOO{this is a test flag}" > /flag && chmod 400 /flag && chown oooser:oooser /flag

COPY src/ .

RUN chmod +x /app/cgi-bin/process.js && chown oooser:www-data /app/cgi-bin/* && chmod 750 /app/cgi-bin/process.js

RUN chown oooser:www-data /app/includes/*

RUN chmod 0500 ccprocessor/run_cc_socket.sh && chown oooser:oooser ccprocessor/run_cc_socket.sh
RUN chmod 0500 ccprocessor/websocket.py && chown oooser:oooser ccprocessor/websocket.py
RUN cd ccprocessor/py8583 && pip3 install -e .

#TODO: add security around /app/includes???

EXPOSE 8888
CMD ["/usr/bin/supervisord","-c","/etc/supervisord.conf"]
#CMD /etc/init.d/apache2 start && /etc/init.d/mysql start && /app/run_web_sys.sh
