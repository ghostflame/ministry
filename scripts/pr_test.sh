#!/bin/bash

METRIC="metric.prediction.test_pr"
PORT=9225
PERIOD=10

BASE=100000
STEP=150
RAND=20

if [ -n "$1" ]; then
    METRIC="$1"
fi


v=$BASE
while [ 1 ]; do

    v=$(($v + $STEP + ($RANDOM % $RAND)))
    echo "$METRIC $v" | nc 127.0.0.1 $PORT && echo "Sent $v"

    sleep $PERIOD
done

