TARGET  = all
VERS    = $(shell ./getversion.sh)

# set some defaults if they are not in the environment
CFGDIR ?= $(DESTDIR)/etc/ministry
LRTDIR ?= $(DESTDIR)/etc/logrotate.d
LOGDIR ?= $(DESTDIR)/var/log/ministry
MANDIR ?= $(DESTDIR)/usr/share/man
DOCDIR ?= $(DESTDIR)/usr/share/doc/ministry
UNIDIR ?= $(DESTDIR)/usr/lib/systemd/system
SSLDIR ?= $(CFGDIR)/ssl
TSTDIR ?= $(CFGDIR)/test

BINS    = ministry ministry-test carbon-copy metric-filter
APPS    = ministry ministry_test carbon_copy metric-filter
SVCS    = ministry carbon-copy metric-filter

CTPATH  = ghostflame/ministry


all:  subdirs
all:  code

with_old_gcc: TARGET = with_old_gcc
with_old_gcc: all

debug: TARGET = debug
debug: all

subdirs:
	@mkdir -p bin

code:
	@cd src && VERS=$(VERS) $(MAKE) $(MFLAGS) $(TARGET)

docker: clean
	docker build -t $(CTPATH):$(VERS) --file dist/Dockerfile .

fedup:
	docker build -t ghostflame/fedora-patched:latest --file dist/Fedora-Dockerfile dist

dockerpush: docker
	docker tag $(CTPATH):$(VERS) $(CTPATH):latest
	docker push $(CTPATH):$(VERS)
	docker push $(CTPATH):latest

install:
	@echo "Making installation directories"
	@mkdir -p $(CFGDIR) $(SSLDIR) $(TSTDIR) $(LRTDIR) $(MANDIR)/man1 $(MANDIR)/man5 $(DOCDIR)
	@cd src && $(MAKE) $(MFLAGS) install; cd ..
	@echo "Creating config and scripts."
	@for a in $(APPS); do \
		install -m644 dist/config/$$d.conf $(CFGDIR)/$dd.conf; \
	done
	@echo "Creating manual pages and docs."
	@for d in $(BINS); do \
		gzip -c dist/man1/$$d.1 > $(MANDIR)/man1/$$d.1.gz; \
		gzip -c dist/man5/$$d.conf.5 > $(MANDIR)/man1/$$d.conf.5.gz; \
		chmod 644 $(MANDIR)/man1/$$d.1.gz $(MANDIR)/man5/$$d.conf.5.gz; \
	done
	@for d in $(SVCS); do \
		install -m644 dist/logrotate/$$d $(LRTDIR)/$$d; \
	done
	@install -m644 dist/ssl/cert.pem $(SSLDIR)/cert.pem
	@install -m600 dist/ssl/key.pem $(SSLDIR)/key.pem
	@cp LICENSE BUGS README.md $(DOCDIR)


unitinstall: install
	@mkdir -p $(UNIDIR)
	@for d in $(SVCS); do \
		install -m644 dist/systemd/$$d.service $(UNIDIR)/$$d.service; \
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
		rm -f $(UNIDIR)/$$d.service $(LRTDIR)/$$d; \
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

