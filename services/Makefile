# PCDS Specific Makefile!
# This Makefile is not part of EPICS, it is only there to support
# a make of the full repository from the top

include $(EPICS_SITE_CONFIG)

DIRS=
DIRS+=gateway

all:

%:
	@set -e; for dir in $(DIRS); do \
		make -C $${dir} $@; \
	done
