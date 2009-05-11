TOP=../..

include $(TOP)/configure/CONFIG

#CROSS_COMPILER_TARGET_ARCHS=RTEMS-beatnik
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library

# Comment this line to build a host-ioc; you'd need to exclude
# several facilities (evr, micro, ...) for this to work.
#BUILDFOR=_RTEMS

#PROD_IOC     += foo

#LIBRARY_IOC  += foo

#multicastBLD_SRCS       = multicast.cpp

#DBD                = foo.dbd

multicastBLDApp_DBD   = base.dbd
#multicastBLDApp_DBD  += multicastBLDSupport.dbd

#multicastBLDApp_SRCS  = multicastBLDApp_registerRecordDeviceDriver.cpp

multicastBLDApp_LIBS += $(EPICS_BASE_IOC_LIBS)

#USR_CFLAGS  = -I$(TOP)/include -I$(RTEMS_BASE)/

#DB              += multicastBLDApp.db

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

