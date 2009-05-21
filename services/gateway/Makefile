# Makefile for the PCDS gateways startup scripts

# This is the Makefile for the scripts

ifndef EPICS_SITE_CONFIG
$(error EPICS_SITE_CONFIG undefined)
endif

include $(EPICS_SITE_CONFIG)

ifndef INSTALL_LOCATION
INSTALL_LOCATION=.
endif

INSTALL_BIN_DIR=$(INSTALL_LOCATION)/bin
INSTALL_LIB_DIR=$(INSTALL_LOCATION)/lib
INSTALL_ETC_DIR=$(INSTALL_LOCATION)/etc

ETC_FILES=$(wildcard *.gwenv *.access *.pvlist)
BIN_FILES=gateway.sh
LIB_FILES=

all: install

install: $(TOOLS_BIN)
	@set -e; if [ ! -d $(INSTALL_BIN_DIR) ]; then mkdir -p $(INSTALL_BIN_DIR); fi
	@set -e; for file in $(BIN_FILES); do \
		echo "INSTALL $${file} (bin)"; \
		cp $${file} $(INSTALL_BIN_DIR)/$${file}; \
	done
	@set -e; if [ ! -d $(INSTALL_LIB_DIR) ]; then mkdir -p $(INSTALL_LIB_DIR); fi
	@set -e; for file in $(LIB_FILES); do \
		echo "INSTALL $${file} (lib)"; \
		cp $${file} $(INSTALL_LIB_DIR)/$${file}; \
	done
	@set -e; if [ ! -d $(INSTALL_ETC_DIR) ]; then mkdir -p $(INSTALL_ETC_DIR); fi
	@set -e; for file in $(ETC_FILES); do \
		echo "INSTALL $${file} (etc)"; \
		cp $${file} $(INSTALL_ETC_DIR)/$${file}; \
	done

clean:
	@set -e; for file in $(BIN_FILES); do \
		if [ -e $(INSTALL_BIN_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_BIN_DIR)/$${file}; \
		fi; \
	done
	@set -e; for file in $(LIB_FILES); do \
		if [ -e $(INSTALL_LIB_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_LIB_DIR)/$${file}; \
		fi; \
	done
	@set -e; for file in $(ETC_FILES); do \
		if [ -e $(INSTALL_ETC_DIR)/$${file} ]; then \
			echo "CLEAN $${file}"; \
			rm $(INSTALL_ETC_DIR)/$${file}; \
		fi; \
	done

distclean realclean:
	@set -e; \
	for dir in $(INSTALL_LIB_DIR) $(INSTALL_ETC_DIR) $(INSTALL_BIN_DIR); do \
		if [ -d $${dir} ]; then \
			echo "REMOVE $${dir}"; \
			rm -r $${dir}; \
		fi; \
	done

.PHONY: all install clean distclean realclean
