CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = log utils regexp net iplist target io mem thread config loop http app
HEADS  = log utils regexp net iplist target io mem thread config loop http app run

VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ../../ministry.spec)
RKV    = app_shared.a

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"'
#LFLAGS = -lm -lcurl -lmicrohttpd

all: $(RKV)

debug: CFLAGS += $(DFLAGS)
debug: $(RKV)

$(RKV): $(OBJS)
	ar crs $(RKV) $(OBJS)
	ranlib $(RKV)

clean:
	rm -f *.o $(RKV)

install:
	@echo "OK."
