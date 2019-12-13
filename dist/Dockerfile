FROM registry.fedoraproject.org/fedora-minimal:30 AS builder

RUN mkdir /app && microdnf install -y gcc make libmicrohttpd-devel libcurl-devel openssl-devel json-c-devel gnutls-devel
COPY Makefile ministry.spec getversion.sh /app/
COPY src/ /app/src
RUN cd /app && mkdir bin && make all


FROM registry.fedoraproject.org/fedora-minimal:30

RUN groupadd -r ministry && \
    useradd -r -g ministry -M -s /bin/sh -d /etc/ministry -c 'Minister for Statistics' ministry && \
    mkdir /var/run/ministry /etc/ministry && \
    chown ministry:ministry /var/run/ministry

RUN microdnf install -y libmicrohttpd libcurl openssl json-c gnutls man-pages man-db mlocate && \
    microdnf clean all && \
	rm -rf /usr/share/X11 /usr/sbin /var/lib/rpm /var/lib/dnf /usr/share/man/man[02346789]*

COPY dist/run.sh /run.sh
COPY dist/config/* /etc/ministry/
COPY dist/man1/* /usr/share/man/man1/
COPY dist/man5/* /usr/share/man/man5/
RUN gzip /usr/share/man/man1/*.1 && gzip /usr/share/man/man5/*.5
COPY --from=builder /app/bin/* /

RUN updatedb

# make the ports available
EXPOSE 8125 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp 9425 9425/udp 9080 9081 9082 9443 9444 9445 2003 2030

CMD [ "/run.sh" ]
