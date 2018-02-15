DIRS   = shared ministry ministry-test carbon-copy
TARGET = unknown

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
	for d in $(DIRS); do cd $$d; $(MAKE) $(MFLAGS) $(TARGET); cd ..; done

.PHONY: all clean debug
