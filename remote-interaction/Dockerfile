FROM python:3.7-alpine

ENV TERM linux

RUN echo "travis_fold:start:DIapt\033[33;1minteractions Dockerfile pkgs\033[0m" && \
    apk update && apk add --no-cache wget curl && \
    echo "\ntravis_fold:end:DIapt\r"

COPY requirements.txt /requirements.txt
RUN echo "travis_fold:start:DIpip\033[33;1minteractions Dockerfile pip\033[0m" && \
    pip install -r requirements.txt && \
    echo "\ntravis_fold:end:DIpip\r"

COPY exploit*.py /

COPY check*.py /

COPY run_all_tests.py /

RUN chmod 777 /*.py


