DIRS   = src
TARGET = all

INSDIR = /opt/ministry
SUBDIR = bin conf logs data

all:
	@mkdir -p bin logs
	@-for d in $(DIRS); do ( cd $$d && $(MAKE) $(MFLAGS) $(TARGET) ); done

debug: TARGET = debug
debug: all

fast:  TARGET = fast
fast:  all

install:
	@mkdir $(INSDIR)
	@-for d in $(SUBDIR); do mkdir -p $(INSDIR)/$(SUBDIR); done
	@echo "Created ministry install location $(INSDIR)"
	@export MIN_INSTALL_DIR=$(INSDIR)
	@export MIN_BIN_DIR=$(INSDIR)/bin
	@-for d in $(DIRS); do ( cd $$d && $(MAKE) $(MFLAGS) install ); done

uninstall:
	@echo "Warning: this may delete your ministry config/log files!"
	@echo "Use make target 'remove' to actually remove ministry."

remove:
	@rm -rf $(INSDIR)

clean:
	@-for d in $(DIRS); do ( cd $$d && $(MAKE) $(MFLAGS) clean ); done
	@rm -f logs/* core*
	@echo "done."

