DIRS    = src test
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
SSLDIR ?= $(CFGDIR)/ssl
TSTDIR ?= $(CFGDIR)/test


all:  subdirs
all:  code

with_old_gcc: TARGET = with_old_gcc
with_old_gcc: all

debug: TARGET = debug
debug: all

subdirs:
	@mkdir -p logs

code:
	@for d in $(DIRS); do cd $$d; $(MAKE) $(MFLAGS) $(TARGET); cd ..; done

docker:
	dist/docker.sh


install:
	@echo "Making installation directories"
	@mkdir -p $(CFGDIR) $(SSLDIR) $(TSTDIR) $(LRTDIR) $(MANDIR)/man1 $(MANDIR)/man5 $(DOCDIR)
	@for d in $(DIRS); do cd $$d; $(MAKE) $(MFLAGS) install; cd ..; done
	@echo "Creating config and scripts."
	@install -m644 dist/ministry.logrotate $(LRTDIR)/ministry
	@install -m644 conf/install.conf $(CFGDIR)/ministry.conf
	@install -m644 test/conf/install.conf $(TSTDIR)/ministry_test.conf
	@install -m644 dist/ssl/cert.pem $(SSLDIR)/cert.pem
	@install -m600 dist/ssl/key.pem $(SSLDIR)/key.pem
	@echo "Creating manual pages and docs."
	@gzip -c dist/ministry.1 > $(MANDIR)/man1/ministry.1.gz
	@chmod 644 $(MANDIR)/man1/ministry.1.gz
	@gzip -c dist/ministry.conf.5 > $(MANDIR)/man5/ministry.conf.5.gz
	@chmod 644 $(MANDIR)/man5/ministry.conf.5.gz
	@cp LICENSE BUGS README.md $(DOCDIR)


unitinstall: install
	@mkdir -p $(UNIDIR)
	@install -m644 dist/ministry.service $(UNIDIR)/ministry.service

initinstall: install
	@mkdir -p $(INIDIR)
	@install -m755 dist/ministry.init $(INIDIR)/ministry


uninstall:
	@echo "Warning: this may delete your ministry config files!"
	@echo "Use make target 'remove' to actually remove ministry."

version:
	@echo $(VERS)

remove:
	@for d in $(DIRS); do cd $$dl; $(MAKE) $(MFLAGS) uninstall; cd ..; done
	@service ministry stop || :
	@rm -rf $(LOGDIR) $(DOCDIR) $(SSLDIR) $(CFGDIR) $(UNIDIR)/ministry.service $(MANDIR)/man1/ministry.1.gz $(MANDIR)/man5/ministry.conf.5.gz $(LRTDIR)/ministry $(INIDIR)/ministry

clean:
	@for d in $(DIRS); do cd $$d; $(MAKE) $(MFLAGS) clean; cd ..; done
	@rm -f logs/* core*
	@echo "done."

.PHONY:  docker

