#!/bin/bash

DO_RESTART=1
OK_RESTART=1
MIN_UPTIME=5

function usage( )
{
    cat <<- EOF
Usage: $0 -h
       $0 [OPTIONS]

Options:
 -h             Print this help
 -o             Run just once (do not restart)
 -e             Restart only on error
 -m             Minimum uptime in seconds to count as working

This script runs ministry as per the available config file.

EOF
    exit ${1:-0}
}


while getopts "hHoem:" o; do
    case $o in
        h|H)    usage
                ;;
        o)      DO_RESTART=0
                ;;
        e)      OK_RESTART=0
                ;;
        m)      MIN_UPTIME=$OPTARG
                ;;
        *)      echo "Unrecognised arg: $o"
                echo ""
                usage 1
                ;;
    esac
done

if [ -z "$CONFIG_URI" ]; then
    echo "No CONFIG_URI environment variable found."
    exit 1
fi

RUN=1

minMsec=$((MIN_UPTIME * 1000))

while [ $RUN -gt 0 ]; do

    ds1=$(date +%s.%N)
    ds1=${ds1:0:23}
    ds1=$(echo "$ds1 * 10" | bc -l | cut -d . -f 1)

    sudo -u ministry -g ministry -- /ministry -iI -c $CONFIG_URI
    ret=$?

    ds2=$(date +%s.%N)
    ds2=${ds2:0:23}
    ds2=$(echo "$ds2 * 10" | bc -l | cut -d . -f 1)

    uptime=$(($ds2 - $ds1))

    # if we weren't up long enough, mark exit as bad
    if [ $uptime -gt $minMsec ]; then
        if [ $ret -eq 0 ]; then
            ret=1
        fi
    fi

    if [ $DO_RESTART -eq 0 ]; then
        RUN=0
    fi

    if [ $ret -eq 0 ]; then
        if [ $OK_RESTART -eq 0 ]; then
            RUN=0
        fi
    fi

    sleep 2
done

exit $ret

