CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = locks mem gc net tcp udp targets data token synth sort m_maths stats selfstats main
HEADS  = data gc mem ministry targets net tcp udp token sort m_maths stats selfstats synth locks typedefs 
VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ../../ministry.spec)

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
RKVS   = ../shared/app_shared.a

WFLAGS = -Wall -Wshadow
IFLAGS = -I. -I../shared
#DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_LOOPS -DDEBUG_SYNTH -DDEBUG_IO -DDEBUG_ENV
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"' -DLOCK_DHASH_MUTEX
LFLAGS = $(TFLAGS) $(IFLAGS) -lm -lcurl -lmicrohttpd

BINDIR = $(DESTDIR)/usr/bin
BIN    = ministry

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

