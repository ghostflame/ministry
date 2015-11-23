CC     = /usr/bin/gcc -Wall -Wshadow

MAXV   = 0
MINV   = 1
MICV   = 4

VER    = $(MAXV).$(MINV).$(MICV)

FILES  = utils log loop thread mem gc net io data stats config main
HEADS  = config data gc io log loop mem ministry net run stats thread typedefs utils

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS)
LFLAGS = $(TFLAGS) $(IFLAGS) -lm

BINDIR = /usr/bin
BIN    = ministry



all:   $(BIN)

debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: $(BIN)

fast:  CFLAGS += -O2
fast:  $(BIN)

version:
	@echo $(VER)

$(BIN): $(HDRS) $(OBJS)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJS)
	@echo "Ministry constructed.  Submit your forms, citizen!"

install:
	@install -m755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed ministry as $(BINDIR)/$(BIN)"

uninstall:
	@echo "Removing ministry binary."
	@rm -f $(BINDIR)/$(BIN)

clean:
	@rm -f core* *.core $(OBJS) $(BIN)

