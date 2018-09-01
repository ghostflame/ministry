CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = targets metric main
HEADS  = targets metric typedefs ministry_test

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)
RKVS   = ../shared/app_shared.a

WFLAGS = -Wall -Wshadow
IFLAGS = -I. -I../shared/
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS)
LFLAGS = $(TFLAGS) $(IFLAGS) -lm -lcurl -lmicrohttpd

BINDIR = $(DESTDIR)/usr/bin
BIN    = ministry-test

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
	@echo "Ministry Tester constructed.  Form generator completed."

install: $(BIN)
	@mkdir -p $(BINDIR)
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed ministry_test as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing ministry_test binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@echo "Tidying up."
	@rm -f core* *.core $(OBJS) $(BIN)

