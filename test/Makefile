DIRS    = src
TARGET  = all

CFGDIR ?= $(DESTDIR)/etc/ministry
LOGDIR ?= $(DESTDIR)/var/log/ministry
MANDIR ?= $(DESTDIR)/usr/share/man
DOCDIR ?= $(DESTDIR)/usr/share/doc/ministry
SSLDIR ?= $(CFGDIR)/ssl
TSTDIR ?= $(CFGDIR)/test

all:  code

with_old_gcc: TARGET = with_old_gcc
with_old_gcc: all

debug: TARGET = debug
debug: all

install: TARGET = install
install: all

uninstall: TARGET = uninstall
uninstall: all

clean: TARGET = clean
clean: all

code:
	@for d in $(DIRS); do cd $$d && $(MAKE) $(MFLAGS) $(TARGET); done

