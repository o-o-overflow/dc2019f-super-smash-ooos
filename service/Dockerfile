from node:8

ENV TERM linux
## for apt to be noninteractive
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true
RUN echo "travis_fold:start:Dapt\033[33;1mservice Dockerfile apt\033[0m" && \
    apt update && \
    apt install -y nginx software-properties-common mysql-server wget emacs \
                   # python 3
                   python3-pip python3-pexpect \
                   # for mysqlclient
                   python3-dev default-libmysqlclient-dev && \
    echo "\ntravis_fold:end:Dapt\r"

RUN echo "travis_fold:start:DIpip\033[33;1mservice Dockerfile pip\033[0m" && \
    pip3 install websockets termcolor mysqlclient && \
    echo "\ntravis_fold:end:DIpip\r"

# nginx setup
COPY setup/default /etc/nginx/sites-available/default
COPY setup/nginx.conf /etc/nginx/nginx.conf

# mysql setup and populate
COPY setup/mysqld.cnf /etc/mysql/mysql.conf.d/mysqld.cnf
COPY setup/install_db.sh /root/install_db.sh
RUN chmod 0700 /root/install_db.sh && chown root:root /root/install_db.sh
COPY setup/ccdbcreate.sql /root/db.sql
RUN chmod 0400 /root/db.sql && chown root:root /root/db.sql

RUN cd /root && /root/install_db.sh

RUN useradd -s /bin/bash -m oooser

WORKDIR /app

COPY src/package*.json ./
RUN npm install express connect-timeout ws

RUN touch /var/log/websocket.log && chmod 0600 /var/log/websocket.log && chown oooser:oooser /var/log/websocket.log
RUN echo "OOO{this is a test flag}" > /flag && chmod 400 /flag && chown oooser:oooser /flag

COPY src/ .

RUN chmod +x run_web_sys.sh

RUN chmod 0500 ccprocessor/run_cc_socket.sh && chown oooser:oooser ccprocessor/run_cc_socket.sh
RUN chmod 0500 ccprocessor/websocket.py && chown oooser:oooser ccprocessor/websocket.py
RUN cd ccprocessor/py8583 && pip3 install -e .

EXPOSE 8888

CMD /etc/init.d/nginx start && /etc/init.d/mysql start && /app/run_web_sys.sh
