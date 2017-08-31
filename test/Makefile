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
with_old_gcc: code

debug: TARGET = debug
debug: code

install: TARGET = install
install: code

uninstall: TARGET = uninstall
uninstall: code

clean: TARGET = clean
clean: code

code:
	@for d in $(DIRS); do cd $$d; $(MAKE) $(MFLAGS) $(TARGET); cd ..; done

