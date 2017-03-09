#!/bin/bash

function plain( )
{
	A=$1
	B=$2
	C=$3
	D=$4

	echo "\
lamp.requests $A
lamp.timings.bet.total.mean $B
tx.bets $C
this.that.theother $D
foo.bar $RANDOM"
}

function statsd( )
{
	A=$1
	B=$2
	C=$3
	D=$4

	echo "\
lamp.requests:$A|ms
lamp.timings.bet.total.mean:$B|ms
this.that.theother:$C|ms
foo.bar:$RANDOM|ms"
}


( while true; do

	a=$(($RANDOM / 20))
	b=$((1000 + ( $RANDOM / 100)))
	c=$((1000 - ( $RANDOM / 32)))
	d=$((100 - ( $RANDOM / 200 )))

	plain $a $b $c $d

	sleep 0.2
done ) | nc 127.0.0.1 9225



