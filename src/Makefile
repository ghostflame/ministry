CC     = /usr/bin/gcc -std=gnu99 $(WFLAGS)

FILES  = utils log loop thread mem gc net tcp udp io target data token synth stats selfstats http auth_ldap config main
HEADS  = config data gc io log loop mem ministry target net tcp udp token run stats selfstats http auth_ldap synth thread typedefs utils
VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ../ministry.spec)

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_LOOPS -DDEBUG_SYNTH -DDEBUG_IO -DDEBUG_ENV
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) -DMINISTRY_VERSION='"$(VERS)"'
LFLAGS = $(TFLAGS) $(IFLAGS) -lm -lcurl -lmicrohttpd -lldap

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
	$(CC) -o $(BIN) $(OBJS) $(LFLAGS)
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

