CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = mem network data main
HEADS  = mem network data archivist

SUBS   = query

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
SUBO   = $(SUBS:%=%/*.o)
RKVS   = ../shared/app_shared.a

EARGS ?=
LARGS ?= -lm -lcurl -lmicrohttpd -ljson-c

WFLAGS = -Wall -Wshadow
#IFLAGS = -I. -I../shared
IFLAGS = -I.
#DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_LOOPS -DDEBUG_SYNTH -DDEBUG_IO -DDEBUG_ENV
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) $(EARGS)
LFLAGS = $(TFLAGS) $(IFLAGS) $(LARGS)

BINDIR = $(DESTDIR)/usr/bin
BIN    = archivist

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
	@for d in $(SUBS); do ( cd $$d && $(MAKE) -j10 $(MFLAGS) $(TARGET) ); done

$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS) $(SUBO) $(RKVS)
	@cp -f $(BIN) ../../bin/
	@echo "Archivist assembled.  Ready to keep your data."

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed archivist as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing archivist binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@for d in $(SUBS); do ( cd $$d && $(MAKE) $(MFLAGS) clean ); done
	@rm -f core* *.core $(OBJS) $(BIN)

