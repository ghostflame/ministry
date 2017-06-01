FROM fedora:25

RUN groupadd -r ministry ; \
useradd  -r -g ministry -M -d /home/ministry -s /sbin/nologin -c 'Minister for Statistics' ministry ; \
dnf install -y procps sudo libcurl libmicrohttpd

ADD src/ministry /ministry
ADD dist/run-ministry.sh /run-ministry.sh
RUN chmod 0755 /ministry /run-ministry.sh

# make the ports available
EXPOSE 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp 8443 9443

CMD ["/run-ministry.sh"]
