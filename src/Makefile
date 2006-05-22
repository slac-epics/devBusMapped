TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#USR_CFLAGS += 

#=============================

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord

#=============================

LIBRARY = devBusMapped
INC     = devBusMapped.h
# <name>.dbd will be created from <name>Include.dbd
DBD		= devBusMapped.dbd

devBusMapped_SRCS += devBusMapped.c
devBusMapped_SRCS += devAiBus.c devAoBus.c devBiBus.c devBoBus.c
devBusMapped_SRCS += devLiBus.c devLoBus.c devMbboBus.c devMbbiBus.c

ifdef EPICS_BASE_IOC_LIBS
devBusMapped_LIBS = $(EPICS_BASE_IOC_LIBS)
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

#suppress non-strict warnings
CMPLR = ANSI
