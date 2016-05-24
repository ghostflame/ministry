CC     = /usr/bin/gcc -std=gnu99 $(WFLAGS)

FILES  = utils log loop thread mem gc net tcp udp io data synth stats selfstats config main
HEADS  = config data gc io log loop mem ministry net tcp udp run stats selfstats synth thread typedefs utils
VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ../ministry.spec)

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) -DMINISTRY_VERSION='"$(VERS)"'
LFLAGS = $(TFLAGS) $(IFLAGS) -lm

BINDIR = $(DESTDIR)/usr/bin
BIN    = ministry

all:   WFLAGS += -Wpedantic
all:   CFLAGS += -O2
all:   $(BIN)

debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: $(BIN)

with_old_gcc:  CFLAGS += -O2
with_old_gcc:  WFLAGS += -pedantic
with_old_gcc:  LFLAGS += -lrt
with_old_gcc:  $(BIN)


$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS)
	@echo "Ministry constructed.  Submit your forms, citizen!"

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed ministry as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing ministry binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@rm -f core* *.core $(OBJS) $(BIN)

