# Ministry
A drop-in replacement for statsd written in threaded C

Etsy's statsd can be found here:  https://github.com/etsy/statsd

It supports statsd's input format:
```
	<metric path>:<value>|<type - g, c or ms>\n
```
As well as a simpler input format:
```
	<metric path> <value>\n
```

Ministry supports counters, timers and gauges, though does not support statsd's
sample rate concept (here and SkyBet we have found that in the few cases where we
do not fully sample a metric, the sample rate is fixed per metric and easily
corrected with a scale() function in graphite).

In order not to balloon in memory over time, Ministry has a basic garbage collection
function, which keeps an eye out for metrics that have stopped reporting.  They are
evicted from memory after a configurable time.

Furthermore, Ministry uses the notion that no data points is different from data of
zero.  Ministry tracks points sent to a metric and does not report a metric with no
points against it.  It will report zero for a metric whose value is zero but which
has received data.


## Ministry Config

Ministry config matches that of Coal (https://github.com/ghostflame/coal), in
that it follows a Section/Variable=Value format, eg:

```
[Main]
basedir = /tmp
tick_usec = 200000

[Logging]
level = info

[Network]
data.udp.port = 9125
statsd.udp.port = 8125
```

It supports including files (though not drop-in directories).  When a file
is included, the section context is carried over, so the included file need
not specify a section.  For example:

conf/ministry.conf:

```
[Logging]
include = conf/log.conf
```

And then conf/log.conf
```
level = info
```

Strings such as log level, yes/no, are case insensitive.


The section values are:

### Main
Controls overall behaviour.

- tick_usec = (integer) usec between main ticks (for clock maintenance).
- daemon    = (integer/string) yes, y, >0 to daemonize
- pidfile   = (path) path to pidfile
- basedir   = (path) working dir to cd to.  Relative paths are from here.


### Logging
Controls the logging code.

- filename  = (path) File to log to
- level     = (string) Log level (debug,info,notice,warn,error,fatal)


### Network
Controls network ports, timeouts.

- timeout     = (integer) Time to consider a TCP connection dead.
- rcv_tmout   = (integer) Seconds for receive timeout - affects signals.
- reconn_msec = (integer) Milliseconds between target reconnect attempts.
- io_msec     = (integer) Milliseconds between io loop checks.

Then there are target specifications, for where to submit data to.

- target.host  = (string) IP address of the graphite target host.
- target.port  = (integer) Graphite target port.

The rest are for one of the 3 types of socket - data, adder or statsd, and
must all be prefixed with one of those three.  The type as a whole, with
both TCP and UDP reception, are enabled by default.

- type.enable      = (integer) 0 or !0, to disable or enable respectively
- type.label       = (string) How this socket is described in logs

After this, everything is by protocol, udp or tcp.

- type.udp.bind    = (string) IP address to bind this socket to
- type.udp.port    = (integer-list) Comma-separated ports to listen on
- type.udp.enable  = (integer) 0 or !0, to disable or enable UDP
- type.tcp.enable  = (integer) 0 or !0, to disable or enable TCP

- type.tcp.bind    = (string) IP address to bind this socket to
- type.udp.port    = (integer) Port to listen for connections on
- type.tcp.backlog = (integer) connection listen backlog.



### Memory
Controls memory management.

- max_mb     = (integer) Max RSS in MB.  Process exits if this is exceeded.
- gc_thresh  = (integer) Submit intervals without data before a path is GC'd.
- hashsize   = (integer) Size of the hash table - affects performance


### Stats
Controls stats submission and prefixes.

These are either for stats, self or adder control and should be prefixed with
one of those types.

- type.threads = (integer) Number of independent processing threads to run
- type.prefix  = (string) Prefix to put before type paths
- type.period  = (integer) Submit interval in milliseconds for this type
- type.offset  = (integer) Submit delay in milliseconds for this type

Self stats loop threads are overridden to 1.


### Synth
Creates synthetic metrics from submitted ones.  These come in blocks of
config ending with a 'done' line.

- target    = (string) metric to create
- source    = (string) first source metric
- source    = (string) second source metric
- source    = (string) ...
- operation = (string) what function to use (sum|diff|ratio|...)
- factor    = (double) factor to multiply result be, defaults to 1


