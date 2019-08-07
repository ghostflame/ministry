#!/bin/bash

PORT=9125
DELAY=''

function submit_ministry( )
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
	if [ -n "$DELAY" ]
	then
		sleep $DELAY
	fi
	done ) | nc 127.0.0.1 $PORT
}


function submit_ministry_one( )
{
    echo "Sending plain (one) to $PORT"
    ( while true; do

      a=$((100 + ( $RANDOM / 200 ) ))
      echo "lamp.timings.bet.total.mean $a"
	if [ -n "$DELAY" ]
	then
		sleep $DELAY
	fi
    done ) | nc 127.0.0.1 $PORT
}

function submit_ministry_single( )
{
    echo "Sending plain (single) to $PORT"
    ( while true; do

      a=$((100 + ( $RANDOM / 200 ) ))
      echo "lamp.timings.bet.total.mean $a"
	if [ -n "$DELAY" ]
	then
		sleep $DELAY
	fi

    done ) | nc 127.0.0.1 $PORT
}

function submit_compat( )
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
	if [ -n "$DELAY" ]
	then
		sleep $DELAY
	fi
	done ) | nc -u 127.0.0.1 8125
}


cmd=$1
shift

if [ -n "$1" ]; then
	DELAY="$1"
fi

case $cmd in
    stats)  PORT=9125
            submit_ministry
            ;;
    single) PORT=9125
            submit_ministry_single
            ;;
    one)    PORT=9125
            submit_ministry_one
            ;;
    paths)  PORT=9225
            submit_ministry
            ;;
    compat) submit_stats
            ;;
    *)      echo "$0 <stats|paths|compat|single|one>"
            ;;
esac

