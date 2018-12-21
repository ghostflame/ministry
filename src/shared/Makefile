CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = log utils regexp curlw net iplist target io mem thread config loop http app
HEADS  = log utils regexp curlw net iplist target io mem thread config loop http app run

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
all: $(RKV)

debug: WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
debug: CFLAGS += $(DFLAGS)
debug: $(RKV)

with_old_gcc: WFLAGS += -pedantic
with_old_gcc: CFLAGS += -O2
with_old_gcc: $(RKV)

$(RKV): $(OBJS)
	ar crs $(RKV) $(OBJS)
	ranlib $(RKV)

clean:
	rm -f *.o $(RKV)

install:
	@echo "OK."
