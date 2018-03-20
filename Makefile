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

BINS    = ministry ministry-test carbon-copy
SVCS    = ministry carbon-copy

all:  subdirs
all:  code

with_old_gcc: TARGET = with_old_gcc
with_old_gcc: all

debug: TARGET = debug
debug: all

subdirs:
	@mkdir -p logs bin

code:
	@cd src && $(MAKE) $(MFLAGS) $(TARGET)

docker:
	dist/docker.sh

install:
	@echo "Making installation directories"
	@mkdir -p $(CFGDIR) $(SSLDIR) $(TSTDIR) $(LRTDIR) $(MANDIR)/man1 $(MANDIR)/man5 $(DOCDIR)
	@cd src && $(MAKE) $(MFLAGS) install; cd ..
	@echo "Creating config and scripts."
	@for d in $(BINS); do \
		install -m644 dist/$$d/install.conf $(CFGDIR)/$$d.conf; \
		@echo "Creating manual pages and docs."; \
		gzip -c dist/$$d/$$d.1 > $(MANDIR)/man1/$$d.1.gz; \
		chmod 644 $(MANDIR)/man1/$$d.1.gz; \
		if [ -f dist/$$d/$$d.conf.5 ]; then \
		    gzip -c dist/$$d/$$d.conf.5 > $(MANDIR)/man5/$$d.conf.5.gz; \
			chmod 644 $(MANDIR)/man5/$$d.conf.5.gz; \
		fi \
	done
	@for d in $(SVCS); do \
		install -m644 dist/$$d/$$d.logrotate $(LRTDIR)/$$d; \
	done
	@install -m644 -o $(USER) dist/ssl/cert.pem $(SSLDIR)/cert.pem
	@install -m600 -o $(USER) dist/ssl/key.pem $(SSLDIR)/key.pem
	@cp LICENSE BUGS README.md $(DOCDIR)


unitinstall: install
	@mkdir -p $(UNIDIR)
	@for d in $(SVCS); do \
		install -m644 dist/$$d/$$d.service $(UNIDIR)/$$d.service; \
	done

initinstall: install
	@mkdir -p $(INIDIR)
	@for d in $(SVCS); do \
		install -m755 dist/$$d/$$d.init $(UNIDIR)/$$d; \
	done

uninstall:
	@echo "Warning: this may delete your ministry config files!"
	@echo "Use make target 'remove' to actually remove ministry."

version:
	@echo $(VERS)

remove:
	@service ministry stop ||:
	@service carbon-copy stop ||:
	@cd src && $(MAKE) $(MFLAGS) uninstall; cd ..
	@for d in $(SVCS); do \
		rm -f $(UNIDIR)/$$d.service $(INIDIR)/$$d $(LRTDIR)/$$d; \
	done
	@for d in $(BINS); do \
		rm -f $(MANDIR)/man1/$$d.1.gz $(MANDIR)/man5/$$d.conf.5.gz; \
	done
	@rm -rf $(LOGDIR) $(DOCDIR) $(SSLDIR) $(CFGDIR)

clean:
	@cd src && $(MAKE) $(MFLAGS) clean
	@rm -f logs/* core* bin/*
	@echo "done."

.PHONY:  docker

