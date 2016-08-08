# Docker image suitable for deploying on CentOS 7
FROM alpine:3.4

# make the ports available
EXPOSE 8125/udp 9125 9125/udp 9225 9225/udp 9325 9325/udp

# give us our config file and log files
VOLUME "/etc/ministry"
VOLUME "/var/log/ministry"

# make a pid dir
RUN mkdir /var/run/ministry

# and what we do - this is the executable
CMD [ "/usr/bin/ministry", "-c", "/etc/ministry/ministry.conf" ]
