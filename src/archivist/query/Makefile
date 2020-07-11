CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = config defs http xform query
HEADS  = local query

RKV    = query_shared.a
EARGS ?=

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. -I..
DFLAGS = -g -pg -ggdb3 -DMINDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) $(EARGS)
#CFLAGS = -c $(TFLAGS) $(IFLAGS) -DMIN_MHD_PASS=0
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
