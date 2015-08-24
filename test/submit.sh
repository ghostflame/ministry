#!/bin/bash


while usleep 1000; do

	a=$(($RANDOM / 20))
	b=$((1000 + ( $RANDOM / 100)))
	c=$((1000 - ( $RANDOM / 32)))

	echo "\
lamp.requests $a
lamp.timings.bet.total.mean $b
this.that.theother $c
foo.bar $RANDOM" | nc -u 127.0.0.1 9125

done
