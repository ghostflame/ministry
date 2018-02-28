FROM alpine:3.7

RUN mkdir /etc/ministry /etc/ministry-test /etc/carbon-copy /lib64
RUN ln -s /lib/ld-musl-x86_64.so.1 /lib64/ld-linux-x86-64.so.2
ADD dist/ministry/install.conf /etc/ministry/ministry.conf
ADD dist/ministry-test/install.conf /etc/ministry-test/ministry-test.conf
ADD dist/carbon-copy/install.conf /etc/carbon-copy/carbon-copy.conf
ADD dist/ministry/ministry.1 /usr/share/man/man1/ministry.1
ADD dist/ministry/ministry.conf.5 /usr/share/man/man5/ministry.conf.5
ADD bin/ministry /ministry
ADD bin/ministry-test /ministry-test
ADD bin/carbon-copy /carbon-copy
ADD dist/run.sh /run.sh

RUN apk update && apk upgrade && apk add shadow libmicrohttpd libcurl
RUN groupadd -r ministry && useradd -r -g ministry -M -s /bin/sh -d /etc/ministry -c 'Minister for Statistics' ministry
RUN mkdir /var/run/ministry && chown ministry:ministry /var/run/ministry

# make the ports available
EXPOSE 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp 9080 9443 2003

CMD [ "/run.sh" ]
