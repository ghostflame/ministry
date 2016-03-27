DIRS    = src
TARGET  = all

VERS    = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ministry.spec)

# set some defaults if they are not in the environment
CFGDIR ?= $(DESTDIR)/etc/ministry
LRTDIR ?= $(DESTDIR)/etc/logrotate.d
LOGDIR ?= $(DESTDIR)/var/log/ministry
MANDIR ?= $(DESTDIR)/usr/share/man
DOCDIR ?= $(DESTDIR)/usr/share/doc/ministry
UNIDIR ?= $(DESTDIR)/usr/lib/systemd/system
INIDIR ?= $(DESTDIR)/etc/init.d


all:  subdirs
all:  code

lrt:  TARGET = lrt
lrt:  all

debug: TARGET = debug
debug: all

fast:  TARGET = fast
fast:  all

subdirs:
	@mkdir -p logs

code:
	@cd src && $(MAKE) $(MFLAGS) $(TARGET)

install:
	@echo "Making installation directories"
	@mkdir -p $(CFGDIR) $(LRTDIR) $(MANDIR)/man1 $(MANDIR)/man5 $(DOCDIR)
	@cd src && $(MAKE) $(MFLAGS) install
	@echo "Creating config and scripts."
	@install -m644 dist/ministry.logrotate $(LRTDIR)/ministry
	@install -m755 conf/install.conf $(CFGDIR)/ministry.conf
	@echo "Creating manual pages and docs."
	@gzip -c dist/ministry.1 > $(MANDIR)/man1/ministry.1.gz
	@chmod 644 $(MANDIR)/man1/ministry.1.gz
	@gzip -c dist/ministry.conf.5 > $(MANDIR)/man5/ministry.conf.5.gz
	@chmod 644 $(MANDIR)/man5/ministry.conf.5.gz
	@cp LICENSE BUGS README.md $(DOCDIR)


unitinstall: install
	@mkdir -p $(UNIDIR)
	@install -m755 dist/ministry.service $(UNIDIR)/ministry.service

initinstall: install
	@mkdir -p $(INIDIR)
	@install -m755 dist/ministry.init $(INIDIR)/ministry


uninstall:
	@echo "Warning: this may delete your ministry config files!"
	@echo "Use make target 'remove' to actually remove ministry."

version:
	@echo $(VERS)

remove:
	@cd src && $(MAKE) $(MFLAGS) uninstall
	@service ministry stop || :
	@rm -rf $(LOGDIR) $(DOCDIR) $(CFGDIR) $(UNIDIR)/ministry.service $(MANDIR)/man1/ministry.1.gz $(MANDIR)/man5/ministry.conf.5.gz $(LRTDIR)/ministry $(INIDIR)/ministry

clean:
	@cd src && $(MAKE) $(MFLAGS) clean
	@rm -f logs/* core*
	@echo "done."


