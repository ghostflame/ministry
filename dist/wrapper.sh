#!/bin/bash

mkdir -p /var/run/ministry
chown ministry:ministry /var/run/ministry

exec /usr/bin/ministry -c /etc/ministry/ministry.conf -p /var/run/ministry/ministry.pid

