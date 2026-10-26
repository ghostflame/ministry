


Parsing scheme:
a/ blah.foo.bar
b/ blah.foo.*
c/ some_function(blah.foo.bar)
d/ some_function(other_function(blah.foo.bar))
e/ multi_function(blah.foo.bar, blah.foo.eek)
f/ weird_function(blah.foo.bar,other_function(blah.foo.bar))
g/ complex_function(blah.foo.bar,other_function(blah.foo.eek),blah.foo.woo)

SO
 Seek ( && seek ,
 start to ( is function name 
 check function name
 seek last )
 enclosed string is the arguments

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


