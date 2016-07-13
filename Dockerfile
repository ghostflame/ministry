# Docker image suitable for deploying on CentOS 7
FROM centos:centos7

# the user things will run as
ENV user ministry

# Systemd needs to be able to access cgroups
VOLUME /sys/fs/cgroup

# Remove the rubbish systemd-container, install pure systemd and remove unrequired unit files
RUN yum clean all; \
    yum -y -q swap -- remove systemd-container systemd-container-libs -- install systemd systemd-libs; \
    (cd /lib/systemd/system/sysinit.target.wants/; \
    for i in *; do [ $i == systemd-tmpfiles-setup.service ] || rm -f $i; done); \
    rm -f /lib/systemd/system/multi-user.target.wants/*; \
    rm -f /etc/systemd/system/*.wants/*; \
    rm -f /lib/systemd/system/local-fs.target.wants/*; \
    rm -f /lib/systemd/system/sockets.target.wants/*udev*; \
    rm -f /lib/systemd/system/sockets.target.wants/*initctl*; \
    rm -f /lib/systemd/system/basic.target.wants/*; \
    rm -f /lib/systemd/system/anaconda.target.wants/*;

# Setup SSH daemon so we can access the container
RUN yum -y -q install openssh-server openssh-clients; \
    ssh-keygen -t dsa -f /etc/ssh/ssh_host_dsa_key -N ''; \
    ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key -N ''; \
    echo 'OPTIONS="-o UseDNS=no -o UsePAM=no -o PasswordAuthentication=yes -o UsePrivilegeSeparation=no"' >> /etc/sysconfig/sshd; \
    systemctl enable sshd.service;

# Install missing standard packages and cleanup repos
RUN yum -y -q install crontabs initscripts man man-pages net-tools sudo telnet vim which iptables-services; \
    yum clean all; \
    rm -f /etc/yum.repos.d/CentOS*; \
    rm -f /etc/yum.repos.d/systemd.repo;

RUN sed -i '\#<include if_selinux_enabled="yes" selinux_root_relative="yes">contexts/dbus_contexts</include>#d' /etc/dbus-1/system.conf
RUN sed -i '\#<include if_selinux_enabled="yes" selinux_root_relative="yes">contexts/dbus_contexts</include>#d' /etc/dbus-1/session.conf

# Add the docker public key to the auth keys for the user
ADD dist/docker_id_rsa.pub /tmp/docker.pub

# Set up the user with passwordless sudo and direct ssh
RUN useradd -d /home/${user} -m -s /bin/bash ${user}; \
    (echo ${user}:${user} | chpasswd); \
    mkdir -p /etc/sudoers.d; \
    echo "${user} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/${user}; \
    chmod 0440 /etc/sudoers.d/${user}; \
    [ ! -d /home/${user}/.ssh ] && mkdir /home/${user}/.ssh; \
    chown -R ${user} /home/${user}/.ssh; \
    chmod 0700 /home/${user}/.ssh; \
    cat /tmp/docker.pub >> /home/${user}/.ssh/authorized_keys; \
    chown ${user} /home/${user}/.ssh/authorized_keys; \
    chmod 0600 /home/${user}/.ssh/authorized_keys; \
    rm -f /tmp/docker.pub; \
    mkdir -p /var/log/ministry; \
    chown ${user}:${user} /var/log/ministry; \
    mkdir /etc/ministry;

# Add docs and config
RUN mkdir -p /usr/share/man/man1 /usr/share/man/man5
ADD dist/ministry.1 /usr/share/man/man1/ministry.1
ADD dist/ministry.conf.5 /usr/share/man/man5/ministry.conf.5
ADD dist/ministry.logrotate /etc/logrotate.d/ministry
ADD conf/install.conf /etc/ministry/ministry.conf
RUN gzip /usr/share/man/man1/ministry.1; \
    gzip /usr/share/man/man5/ministry.conf.5;

# Stick these in root
ADD LICENSE /LICENSE
ADD BUGS /BUGS
ADD README.md /README.md

# Add the service file
ADD dist/ministry.service /usr/lib/systemd/system/ministry.service

# Add the single binary
ADD src/ministry /usr/bin/ministry

# make the ports available
EXPOSE 8125
EXPOSE 9125
EXPOSE 9225
EXPOSE 9325

# and light it up
RUN systemctl enable ministry.service


