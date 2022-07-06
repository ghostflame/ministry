* PMET Prometheus Metrics

** Overview

Pmet provides each of the main types of metrics:
 - untyped
 - counter
 - gauge
 - summary
 - histogram

 It provides three update mechanisms:
  - direct update by function
  - read a variable from a pointer-to-double
  - run a callback function

*** Structure

Pmet creates a 'metric' structure, which has the properties of type, path and help string.  This is unique within a copy of Ministry.  All metrics belong to one of a number of 'sources' (invisible to Prometheus, only used for control).

Then one or more 'items' are created against that metric, each being a distinct label set.  They are easily cloned.  An item has default labels, which can be supplemented on direct update.

** Usage

First you should create a source, which is just a named grouping of metrics.  The idea of sources is that they can be enabled and
disabled, turning on and off the metrics inside them collectively.

Then create a metric within the source.  Then for each set of labels inside it, create an item.  They can be easily cloned, and the function 'pmet_clone_vary' allows you to create a first one with appropriate labels, then clone it, varying one label as you go.
Eg, create an item for each log level, varying the log level label as you go.
