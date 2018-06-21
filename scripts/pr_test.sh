#!/bin/bash

PERIOD=10
PORT=9225
METRIC="metric.prediction.test_pr"

BASE=100000
STEP=150
RAND=20


v=$BASE
while [ 1 ]; do

    v=$(($v + $STEP + ($RANDOM % $RAND)))
    echo "$METRIC $v" | nc 127.0.0.1 $PORT && echo "Sent $v"

    sleep $PERIOD
done

