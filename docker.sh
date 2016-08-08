#!/bin/bash

tmpdir=$(mktemp -p /tmp -d ministry.docker.tmp.XXXXXXXX)
DESTDIR=$tmpdir make all unitinstall
cp Dockerfile $tmpdir
docker build -t ghostflame/ministry $tmpdir
echo "rm -rf $tmpdir"

