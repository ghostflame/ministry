CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = mem net tcp udp hash relay main
HEADS  = mem carbon_copy net tcp udp hash relay selfstats typedefs 

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
RKVS   = ../shared/app_shared.a

WFLAGS = -Wall -Wshadow
IFLAGS = -I. -I../shared
#DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_IO -DDEBUG_RELAY
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS)
LFLAGS = $(TFLAGS) $(IFLAGS) -lm -lcurl -lmicrohttpd

BINDIR = $(DESTDIR)/usr/bin
BIN    = carbon-copy

all:   WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
all:   CFLAGS += -O2
all:   $(BIN)

debug: WFLAGS += -Wpedantic -Wextra -Wno-unused-parameter
debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: $(BIN)

with_old_gcc:  CFLAGS += -O2
with_old_gcc:  WFLAGS += -pedantic
with_old_gcc:  LFLAGS += -lrt
with_old_gcc:  $(BIN)


$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS) $(RKVS)
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
	@rm -f core* *.core $(OBJS) $(BIN)

