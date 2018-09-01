CC     = /usr/bin/gcc -std=c11 $(WFLAGS)

FILES  = log utils regexp curlw net iplist target io mem thread config loop http app
HEADS  = log utils regexp curlw net iplist target io mem thread config loop http app run

VERS  ?= 0.1.0
RKV    = app_shared.a

OBJS   = $(FILES:%=%.o)
HDRS   = $(HEADS:%=%.h)

WFLAGS = -Wall -Wshadow -Wextra -Wpedantic -Wno-unused-parameter
IFLAGS = -I. 
DFLAGS = -g -pg -ggdb3 -DDEBUG
TFLAGS = -pthread
CFLAGS = -c $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"'
#CFLAGS = -c $(TFLAGS) $(IFLAGS) -DVERSION_STRING='"$(VERS)"' -DMIN_MHD_PASS=0
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
