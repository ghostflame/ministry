#!/bin/bash

PORT=9125

function submit_plain( )
{
	echo "Sending plain to $PORT"
	( while true; do

		a=$(($RANDOM / 20))
		b=$((1000 + ( $RANDOM / 100)))
		c=$((1000 - ( $RANDOM / 32)))
		d=$(($RANDOM / 200))

		echo "\
lamp.requests $a
lamp.timings.bet.total.mean $b
this.that.theother $c
tx.bets $d
basic.counter 1
foo.bar $RANDOM" 
	done ) | nc 127.0.0.1 $PORT
}

function submit_stats( )
{
	echo "Sending stats to 9125."
	( while true; do

		a=$(($RANDOM / 20))
		b=$((1000 + ( $RANDOM / 100)))
		c=$((1000 - ( $RANDOM / 32)))
		d=$(($RANDOM / 200))

		echo "\
lamp.requests:$a|ms
lamp.timings.bet.total.mean:$b|ms
this.that.theother:$c|ms
tx.bets:$d|ms
foo.bar:$RANDOM|ms"
	done ) | nc -u 127.0.0.1 8125
}

if [ "$1" == "stats" ]; then
	submit_stats
else
	if [ "$1" == "paths" ]; then
		PORT=9225
	fi
	submit_plain
fi

