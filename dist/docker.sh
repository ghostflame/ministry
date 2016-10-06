#!/bin/bash

make clean all
version=$(sed -rn 's/^Version:\t(.*)/\1/p' ministry.spec)
docker build -t ghostflame/ministry:$version --file dist/Dockerfile .


