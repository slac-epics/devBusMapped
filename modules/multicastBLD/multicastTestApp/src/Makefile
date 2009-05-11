TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

PROD_IOC = multicastTest

#DBD += multicastTest.dbd
# multicastTest.dbd will be made up from these files:
multicastTest_DBD += base.dbd
#include definitions for any other support applications needed

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
multicastTest_SRCS += multicastTest_registerRecordDeviceDriver.cpp
multicastTest_SRCS_DEFAULT += multicastTestMain.cpp
multicastTest_SRCS_vxWorks += -nil-

#The following adds support from base/src/vxWorks
multicastTest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
#multicastTest_LIBS_RTEMS += foo

multicastTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

