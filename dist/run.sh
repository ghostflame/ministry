#!/bin/sh

if [ -z "$1" ]; then
    prog='ministry'
else
    prog=$1
    shift
fi

case $prog in
    ministry|ministry-test|carbon-copy)
            ;;
    *)
            exit 1
            ;;
esac

# and execute it
# fix the config - mount over it if you want
# fix stdout logging - no local logging!
su ministry -c "/${prog} -V $*"
