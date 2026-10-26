ODIRS  = shared
#ADIRS  = ministry ministry-test carbon-copy metric-filter archivist
ADIRS  = ministry ministry-test carbon-copy metric-filter
TARGET = unknown
VERS  ?= 0.0.1

#EARGS  = -DMTYPE_TRACING
EARGS  = 

# which C standard?
STD   ?= c23

LARGS  = -lm -lcurl -lmicrohttpd -ljson-c -lgnutls

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
	for d in $(ODIRS); do cd $$d; EARGS="$(EARGS)" VERS=$(VERS)     STD=$(STD) $(MAKE) $(MFLAGS) $(TARGET); cd ..; done
	for d in $(ADIRS); do cd $$d; EARGS="$(EARGS)" LARGS="$(LARGS)" STD=$(STD) $(MAKE) $(MFLAGS) $(TARGET); cd ..; done

.PHONY: all clean debug
