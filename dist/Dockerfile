FROM fedora:30

#RUN apk update && apk upgrade && apk add shadow libmicrohttpd libcurl
RUN dnf update -y && \
    dnf install -y shadow-utils libmicrohttpd libcurl openssl json-c man-pages man-db mlocate && \
    rm -rf /var/log/dnf*.log /var/lib/dnf /var/cache/dnf

RUN groupadd -r ministry && \
    useradd -r -g ministry -M -s /bin/sh -d /etc/ministry -c 'Minister for Statistics' ministry && \
     mkdir /var/run/ministry /etc/ministry && \
	 chown ministry:ministry /var/run/ministry

COPY dist/run.sh /run.sh
COPY dist/config/* /etc/ministry/
COPY dist/man1/* /usr/share/man/man1/
COPY dist/man5/* /usr/share/man/man5/
COPY bin/* /

RUN updatedb

# make the ports available
EXPOSE 8125 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp 9080 9443 2003

CMD [ "/run.sh" ]
