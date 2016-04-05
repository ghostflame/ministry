CC     = /usr/bin/gcc -Wall -Wshadow -pedantic

FILES  = utils log loop thread mem gc net io data synth stats config main
HEADS  = config data gc io log loop mem ministry net run stats synth thread typedefs utils

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG -DDEBUG_SYNTH
TFLAGS = -pthread
CFLAGS = $(TFLAGS) $(IFLAGS)
LFLAGS = $(TFLAGS) $(IFLAGS) -lm

BINDIR = $(DESTDIR)/usr/bin
BIN    = ministry



all:   $(BIN)

debug: CFLAGS += $(DFLAGS)
debug: LFLAGS += $(DFLAGS)
debug: $(BIN)

fast:  CFLAGS += -O2
fast:  $(BIN)

lrt:   LFLAGS += -lrt
lrt:  $(BIN)


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

