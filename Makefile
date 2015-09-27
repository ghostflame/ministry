DIRS   = src
TARGET = all

LOGDIR = /var/log/ministry
CFGDIR = /etc/ministry
INISCR = /etc/init.d/ministry


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
	@install -m755 scripts/ministry $(INISCR)
	@install -m755 conf/basic.conf $(CFGDIR)/ministry.conf

uninstall:
	@echo "Warning: this may delete your ministry config/log files!"
	@echo "Use make target 'remove' to actually remove ministry."

remove:
	@cd src && $(MAKE) $(MFLAGS) uninstall
	@rm -rf $(LOGDIR) $(CFGDIR) $(INISCR)

clean:
	@cd src && $(MAKE) $(MFLAGS) clean
	@rm -f logs/* core*
	@echo "done."


