#!/bin/bash

qa="blah.foo.bar"
qb="blah.foo.*"
qc="blah.foo.bar,blah.foo.eek"
qd="blah.foo.bar,blah.foo.eek,blah.eek.*"
qe="some_function(blah.foo.bar)"
qf="some_function(other_function(blah.foo.bar))"
qg="multi_function(blah.foo.bar, blah.foo.eek)"
qh="weird_function(blah.foo.bar,other_function(blah.foo.bar))"
qi="complex_function(blah.foo.bar,other_function(blah.foo.eek),blah.foo.woo)"

SARG='((:?[a-z0-9_]+|*)(:?\.(:?[a-z0-9_]+|*))*)(:?,(.*))?'

echo $qa | sed -r "s/$SARG/\1/i"
echo $qb | sed -r "s/$SARG/\1/i"

echo $qc | sed -r "s/$SARG/\1/i"
echo $qc | sed -r "s/$SARG/\2/i"

