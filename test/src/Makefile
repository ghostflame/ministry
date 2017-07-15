CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = utils regexp log loop thread mem net io metric target config main
HEADS  = config net io log loop mem target run thread metric typedefs regexp utils ministry_test
VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ../../ministry.spec)

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"'
LFLAGS = $(TFLAGS) $(IFLAGS) -lm -lcurl

BINDIR = $(DESTDIR)/usr/bin
BIN    = ministry_test

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
	@echo "Ministry Tester constructed.  Form generator completed."

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed ministry_test as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing ministry_test binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@rm -f core* *.core $(OBJS) $(BIN)

