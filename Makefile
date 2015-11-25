DIRS   = src
TARGET = all

LOGDIR = /var/log/ministry
CFGDIR = /etc/ministry
INISCR = /etc/init.d/ministry
MANDIR = /usr/share/man

VERS   = $(shell sed -rn 's/^Version:\t(.*)/\1/p' ministry.spec)

SUBDIR = bin conf logs data

all:  subdirs
all:  code

debug: TARGET = debug
debug: all

fast:  TARGET = fast
fast:  all

subdirs:
	@mkdir -p logs

code:
	@cd src && $(MAKE) $(MFLAGS) $(TARGET)

install:
	@echo "Creating ministry install locations"
	@mkdir -p $(LOGDIR) $(CFGDIR)
	@cd src && $(MAKE) $(MFLAGS) install
	@echo "Creating config and init script."
	@install -m755 dist/ministry.init $(INISCR)
	@install -m755 conf/basic.conf $(CFGDIR)/ministry.conf
	@gzip -c dist/ministry.1 > $(MANDIR)/man1/ministry.1.gz
	@chmod 644 $(MANDIR)/man1/ministry.1.gz
	@gzip -c dist/ministry.conf.5 > $(MANDIR)/man5/ministry.conf.5.gz
	@chmod 644 $(MANDIR)/man5/ministry.conf.5.gz

uninstall:
	@echo "Warning: this may delete your ministry config/log files!"
	@echo "Use make target 'remove' to actually remove ministry."

version:
	@echo $(VERS)

remove:
	@cd src && $(MAKE) $(MFLAGS) uninstall
	@rm -rf $(LOGDIR) $(CFGDIR) $(INISCR) $(MANDIR)/man1/ministry.1.gz $(MANDIR)/man5/ministry.conf.5.gz

clean:
	@cd src && $(MAKE) $(MFLAGS) clean
	@rm -f logs/* core*
	@echo "done."


