#!/bin/bash

function submit_adder( )
{
	echo "Sending paths to 9225"
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
	done ) | nc 127.0.0.1 9225
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
	done ) | nc 127.0.0.1 9125
}

if [ "$1" == "stats" ]; then
	submit_stats
else
	submit_adder
fi

