CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = mem main
HEADS  = mem metric_filter typedefs

SUBS   =

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
SUBO   = $(SUBS:%=%/*.o)
RKVS   = ../shared/app_shared.a

EARGS ?=
LARGS ?= -lm -lcurl -lmicrohttpd -ljson-c

WFLAGS = -Wall -Wshadow
#IFLAGS = -I. -I../shared
IFLAGS = -I.
#DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_IO -DDEBUG_RELAY
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) $(EARGS)
LFLAGS = $(TFLAGS) $(IFLAGS) $(LARGS)

BINDIR = $(DESTDIR)/usr/bin
BIN    = metric-filter

all:   WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
all:   CFLAGS += -O2
all:   TARGET = all
all:   subs
all:   $(BIN)

debug: WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: TARGET = debug
debug: subs
debug: $(BIN)

with_old_gcc:  CFLAGS += -O2
with_old_gcc:  WFLAGS += -pedantic
with_old_gcc:  LFLAGS += -lrt
with_old_gcc:  TARGET = with_old_gcc
with_old_gcc:  subs
with_old_gcc:  $(BIN)

subs:
	@for d in $(SUBS); do ( cd $$d && $(MAKE) $(MFLAGS) $(TARGET) ); done

$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS) $(SUBO) $(RKVS)
	@cp -f $(BIN) ../../bin/
	@echo "Metric-filter built - ready to throw things away."

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed metric-filter as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing metric-filter binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@for d in $(SUBS); do ( cd $$d && $(MAKE) $(MFLAGS) clean ); done
	@rm -f core* *.core $(OBJS) $(BIN)
