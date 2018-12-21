DIRS   = shared ministry ministry-test carbon-copy
TARGET = unknown
VERS  ?= 0.0.1

#EARGS  = -DMTYPE_TRACING
EARGS  =

all: TARGET = all
all: subdirs

install: TARGET = install
install: subdirs

clean: TARGET = clean
clean: subdirs
	@rm -f ../bin/*

debug: TARGET = debug
debug: subdirs

subdirs:
	for d in $(DIRS); do cd $$d; EARGS="$(EARGS)" VERS=$(VERS) $(MAKE) $(MFLAGS) $(TARGET); cd ..; done

.PHONY: all clean debug
