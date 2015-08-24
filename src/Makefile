CC     = /usr/bin/gcc -Wall -Wshadow

MAXV   = 0
MINV   = 0
MICV   = 1

VER    = $(MAXV).$(MINV).$(MICV)

BINDIR = ../bin

FILES  = utils log loop thread mem net data stats config main

OBJS   = $(FILES:%=%.o)

IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS)
LFLAGS = $(TFLAGS) $(IFLAGS) -lm

BIN    = ministry



all:   $(BIN)

debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: $(BIN)

fast:  CFLAGS += -O2
fast:  $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BINDIR)/$(BIN) $(LFLAGS) $(OBJS)
	@echo "Ministry constructed.  Submit your forms, citizen!"

install:
	@install -m755 $(BINDIR)/$(BIN) $(MIN_BIN_DIR)/$(BIN)
	@echo "Installed ministry as $(MIN_BIN_DIR)/$(BIN)"

clean:
	@rm -f core* *.core $(OBJS) $(BINDIR)/$(BIN)


