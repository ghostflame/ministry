#!/bin/bash

NC=$(which nc)
PORT=12003
FILE=''
GREP=''
VERB=0

function usage( ) {
	cat <<- END
Usage:  endpoint.sh -H
        endpoint.sh [-f <file>] [-p <port>] [-g <regex>]

Create a graphite endpoint using netcat.  It can filter what
comes out and capture it if desired.

Options:
 -H            Print this help.
 -f <file>     Write output to file.
 -p <port>     Listen on specified port.  Default is $PORT.
 -g <regex>    Filter using this regex.
 -v            Be verbose.

END
	exit 0
}

SECONDS=$(date +%s)

function logit( ) {

	if [ $VERB -gt 0 ]; then
		echo "[$SECONDS] $*"
	fi
}


while getopts "Hhvp:g:f:" arg; do
	case $arg in
		H|h)	usage
				;;
		f)		FILE=$OPTARG
				;;
		p)		PORT=$OPTARG
				;;
		g)		GREP=$OPTARG
				;;
		v)		VERB=1
				;;
	esac
done

if [ -n "$GREP" ]; then
	if [ -n "$FILE" ]; then
		logit "Listening on port $PORT, filtering for '$GREP', output copied to $FILE." 
		nc -kln $PORT | egrep --line-buffered $GREP | tee $FILE
	else
		logit "Listening on port $PORT, filtering for '$GREP'." 
		nc -kln $PORT | egrep --line-buffered $GREP
	fi
else

	if [ -n "$FILE" ]; then
		logit "Listening on port $PORT, output copied to $FILE." 
		nc -kln $PORT | tee $FILE
	else
		logit "Listening on port $PORT."
		nc -kln $PORT
	fi
fi

