TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

PROD_IOC_RTEMS = PhaseCavityTiming

DBD += PhaseCavityTiming.dbd
# PhaseCavityTiming.dbd will be made up from these files:
PhaseCavityTiming_DBD += base.dbd
#include definitions for any other support applications needed
PhaseCavityTiming_DBD += vmeDigiApp.dbd
PhaseCavityTiming_DBD += vmeDigiSupport.dbd
#PhaseCavityTiming_DBD += IP231.dbd
#PhaseCavityTiming_DBD += IP330.dbd
#PhaseCavityTiming_DBD += Xy2445.dbd
#PhaseCavityTiming_DBD += Xy2440.dbd
#PhaseCavityTiming_DBD += evrSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
PhaseCavityTiming_SRCS += PhaseCavityTiming_registerRecordDeviceDriver.cpp
PhaseCavityTiming_SRCS_DEFAULT += PhaseCavityTimingMain.cpp
PhaseCavityTiming_SRCS_vxWorks += -nil-

#The following adds support from base/src/vxWorks
#PhaseCavityTiming_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
#PhaseCavityTiming_LIBS += xxx

PhaseCavityTiming_LIBS_RTEMS += devBusMapped
#PhaseCavityTiming_LIBS_RTEMS += IP231
#PhaseCavityTiming_LIBS_RTEMS += IP330
#PhaseCavityTiming_LIBS_RTEMS += Xy2445
#PhaseCavityTiming_LIBS_RTEMS += Xy2440
#PhaseCavityTiming_LIBS_RTEMS += evrSupport
PhaseCavityTiming_LIBS_RTEMS += vmeDigiDevSup
PhaseCavityTiming_LIBS_RTEMS += vmeDigi
PhaseCavityTiming_LIBS_RTEMS += Ipac
PhaseCavityTiming_LIBS_RTEMS += devMrfEr
PhaseCavityTiming_LIBS_RTEMS += generalTime
PhaseCavityTiming_LIBS_RTEMS += mrfVme64x
PhaseCavityTiming_LIBS_RTEMS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

