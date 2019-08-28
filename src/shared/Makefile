CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

SUBS   = config log mem pmet http ha utils strings io target net

FILES  = iter regexp curlw iplist thread loop app
HEADS  = iter regexp curlw iplist thread loop app run json

VERS  ?= 0.1.0
RKV    = app_shared.a
EARGS ?=

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
SUBO   = $(SUBS:%=%/*.o)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) $(EARGS)
#CFLAGS = -c $(TFLAGS) $(IFLAGS) -DMIN_MHD_PASS=0

all: WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
all: CFLAGS += -O2
all: TARGET = all
all: subs
all: $(RKV)

debug: WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
debug: CFLAGS += $(DFLAGS)
debug: TARGET = debug
debug: subs
debug: $(RKV)

with_old_gcc: WFLAGS += -pedantic
with_old_gcc: CFLAGS += -O2
with_old_gcc: TARGET = with_old_gcc
with_old_gcc: subs
with_old_gcc: $(RKV)


subs:
	for d in $(SUBS); do ( cd $$d && EARGS=$(EARGS) VERS=$(VERS) $(MAKE) $(MFLAGS) $(TARGET) ); done

$(RKV): $(OBJS)
	ar crs $(RKV) $(OBJS) $(SUBO)
	ranlib $(RKV)

clean:
	for d in $(SUBS); do ( cd $$d && EARGS=$(EARGS) $(MAKE) $(MFLAGS) clean ); done
	rm -f *.o $(RKV)

install:
	@echo "OK."

