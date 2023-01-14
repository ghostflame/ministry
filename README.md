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


## Performance

Ministry was designed with quite high performance in mind, mainly as an exercise in
threaded C but also to fill a need.  As such, it uses quite a few threads, but there
are several design principles at work:

* One thread has one job
* Threads never re-attach
* (Mostly) threads do not wait for other threads
* Anything that might block is off in its own thread

Shared resources (like the data on a path) are controlled with spinlocks or mutexes.

In straight-line testing, ie:  cat file-full-of-lines | nc localhost 9225, ministry
can achieve about 8.3M lines/sec into one path on a 3.4GHz Intel desktop chip (and
a local SSD drive).  A second input on the same path increases it to around 13.8M
lines/sec, but with 4 simultaneous inputs on separate paths, a full 32.5M lines/
sec was seen.  This was on an adder path - the simplest type.

Reporting on stats is the biggest CPU task, and this is done by separate threads to
prevent it interfering with data reception.  The number of these is configurable
and should be scaled to the host, as at metric generation time Ministry will go as
fast as it can.

In the midst of stats processing there is a sort - and it's unavoidable.  There is
no easy way to get the n'th percentile without a sort.  So the stats threads will
routinely do far more work that the path or gauge threads.

To reduce the impact of generating stats, and to make them as coherent as possible
in time, Ministry performs a two-phase pass.

In the first pass, the current stats are 'stolen', from an incoming struct to a
processing struct.  This freezes the state of the metric while allowing new data to
accrue while processing happens.

Then the second, slower, pass happens, where the data is crunched.  As long as this
does not drift into the next interval, there's no problem.  Ministry does not have
a specific protection against this, but it does report on the time taken to do its
stats, and the percentage of the interval used.



## Ministry Config

Ministry config is in sections, with each line of the form: var = val

[Section]
Variable = Value

It can include a file, and the section context is carried over into that file.
For example:

conf/ministry.conf:

```
[Logging]
include = conf/log.conf
```

And then conf/log.conf
```
level = info
```

Ministry ships with a self-documenting config file with every option mentioned and
explained in it, as well as a manual page for its config file.

Ministry can take HTTP URI's as config sources, even for includes.


## Other Apps

Ministry ships with three other apps - carbon-copy, metric-filter and ministry-test.  Each comes with its own
default config file and man pages.

### Carbon-Copy

This is a drop-in replacement for carbon-relay or carbon-c-relay.  It copies metrics to multiple targets, with
hashing or duplication as desired.

### Metric-Filter

This performs a related function to carbon-relay, but does filtering based on JSON files, allowing regexes to
determine which hosts may send which metrics.  Good for handling spammy sources, or developers cutting their
own sending clients.

### Ministry-Test

This is used primarily in development, but is provided as a means to load-test your installation and get a
sense of the resources needed for your anticipated metrics load.  It generates metrics defined in config, with
quasi-realistic behaviour and several different models for a metric.


## Build

Ministry is provided as a tar.gz release, but it relatively easily built into an RPM.  However, given its
dependency on relatively modern versions of libcurl and libmicrohttpd, and the lack of those in enterprise
versions of linux such as RHEL7, it is more normal to run ministry as a container.  It has no meaningful
interaction with disk, and logs to stdout.   It can accept config from file or HTTP or environment.  It makes
for a good candidate for containerisation.

This being true, most releases are built as containers as well, with the path:
```
hub.docker.com/ghostflame/ministry
```
The latest tag is kept up to date with each release.

