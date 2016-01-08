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
this.that.theother $C
tx.bets $D
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
tx.bets:$D|ms
foo.bar:$RANDOM|ms"
}


( while usleep 1000000; do

	a=$(($RANDOM / 20))
	b=$((1000 + ( $RANDOM / 100)))
	c=$((1000 - ( $RANDOM / 32)))
	d=$(($RANDOM / 200))

	plain $a $b $c $d
done ) | nc 127.0.0.1 9225

