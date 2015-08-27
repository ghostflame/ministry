#!/bin/bash

function plain( )
{
	A=$1
	B=$2
	C=$3

	echo "\
lamp.requests $A
lamp.timings.bet.total.mean $B
this.that.theother $C
foo.bar $RANDOM" 
}

function statsd( )
{
	A=$1
	B=$2
	C=$3

	echo "\
lamp.requests:$A|ms
lamp.timings.bet.total.mean:$B|ms
this.that.theother:$C|ms
foo.bar:$RANDOM|ms"
}


( while usleep 1000; do

	a=$(($RANDOM / 20))
	b=$((1000 + ( $RANDOM / 100)))
	c=$((1000 - ( $RANDOM / 32)))

	plain $a $b $c
done ) | nc 127.0.0.1 9125

