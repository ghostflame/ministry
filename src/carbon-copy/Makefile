CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = mem hash host selfstats main
HEADS  = mem carbon_copy hash host selfstats typedefs

SUBS   = relay

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
SUBO   = $(SUBS:%=%/*.o)
RKVS   = ../shared/app_shared.a

EARGS ?=
LARGS ?= -lm -lcurl -lmicrohttpd -ljson-c -lgnutls

WFLAGS = -Wall -Wshadow
#IFLAGS = -I. -I../shared
IFLAGS = -I.
DFLAGS = -g -pg -ggdb3 -DMINDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) $(EARGS)
LFLAGS = $(TFLAGS) $(IFLAGS) $(LARGS)

BINDIR = $(DESTDIR)/usr/bin
BIN    = carbon-copy

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
	@for d in $(SUBS); do ( cd $$d && $(MAKE) -j4 $(MFLAGS) $(TARGET) ); done

$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS) $(SUBO) $(RKVS)
	@cp -f $(BIN) ../../bin/
	@echo "Carbon-copy built - ready to duplicate."

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed carbon-copy as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing carbon-copy binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@for d in $(SUBS); do ( cd $$d && $(MAKE) $(MFLAGS) clean ); done
	@rm -f core* *.core $(OBJS) $(BIN)

