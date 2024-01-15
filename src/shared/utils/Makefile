CC     = /usr/bin/gcc -std=c18 $(WFLAGS)

FILES  = json loop iter time utils var_val
HEADS  = loop iter utils

RKV    = utils_shared.a
EARGS ?=

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. -I..
DFLAGS = -g -pg -ggdb3 -DMINDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) $(EARGS)

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
