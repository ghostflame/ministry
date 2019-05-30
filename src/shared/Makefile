CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

SUBS   = pmet

FILES  = log utils stringutils iter regexp curlw net iplist target io mem thread config loop http http_calls ha app
HEADS  = log utils stringutils iter regexp curlw net iplist target io mem thread config loop http pmet http_calls ha app run json

VERS  ?= 0.1.0
RKV    = app_shared.a
EARGS ?=

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"' $(EARGS)
#CFLAGS = -c $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"' -DMIN_MHD_PASS=0
#LFLAGS = -lm -lcurl -lmicrohttpd

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
	for d in $(SUBS); do ( cd $$d && EARGS=$(EARGS) $(MAKE) $(MFLAGS) $(TARGET) ); done

$(RKV): $(OBJS)
	ar crs $(RKV) $(OBJS) */*.o
	ranlib $(RKV)

clean:
	for d in $(SUBS); do ( cd $$d && EARGS=$(EARGS) $(MAKE) $(MFLAGS) clean ); done
	rm -f *.o $(RKV)

install:
	@echo "OK."
