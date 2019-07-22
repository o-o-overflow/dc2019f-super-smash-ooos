ARG SERVICE=build--this--with--a--build-arg
FROM $SERVICE

RUN apt install bsdmainutils

# copy source as original for comparisons
RUN mkdir /tmp/app && cp -r /app /tmp/

COPY check_filetype.sh /
COPY check_filetype.sh /check_invariance.sh

RUN chmod +x /*.sh

