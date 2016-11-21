FROM fedora:24

RUN mkdir /etc/ministry
ADD conf/install.conf /etc/ministry/ministry.conf
ADD src/ministry /usr/bin/ministry
ADD dist/ministry.service /etc/systemd/system/ministry.service

RUN groupadd -r ministry ;\
useradd  -r -g ministry -M -d /etc/ministry -s /sbin/nologin -c 'Minister for Statistics' ministry ;\
mkdir /var/log/ministry ;\
chown ministry:ministry /var/log/ministry ;\
dnf install -y procps systemd-container ;\
ln -s /etc/systemd/system/ministry.service /etc/systemd/system/multi-user.target.wants/ministry.service

# make the ports available
EXPOSE 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp

CMD [ "/usr/sbin/init" ]

