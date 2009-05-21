#!/bin/sh
#
# This script setup a working EPICS environment. It gives access
# to EDM, VDCT and other development tools that are part of EPICS.
#
# Remi Machet, Copyright @2009 SLAC National Accelerator Laboratory
#
# Since one may want to use stable tools while working
# on a development EPICS this script will try to detect 
# the version to use using the following rules:
# -If EPICS_TOOLS_SITE_TOP is defined it will use it
# -If not it will read it from EPICS_SITE_CONFIG

load_epics_config_var() {
	export $1=$(make -f ${TOOLS_DIR}/lib/Makefile.displayvar $1)
}

# EPICS_SITE_CONFIG should be defined
if [ -z "${EPICS_SITE_CONFIG}" ]; then
	echo "ERROR: EPICS_SITE_CONFIG must be defined."
	return 1
fi

EPICS_SITE_TOP=$(dirname ${EPICS_SITE_CONFIG})

# Now we detect the version of the tools and deduct the path from it
TOOLS_MODULE_VERSION=$(grep -E '^[ ]*TOOLS_MODULE_VERSION[ ]*=' ${EPICS_SITE_CONFIG} | sed -e 's/^[ ]*TOOLS_MODULE_VERSION[ ]*=[ ]*\([^# ]*\)#*.*$/\1/')
EPICS_SITE_TOP=$(make -f ${EPICS_SITE_TOP}/tools/${TOOLS_MODULE_VERSION}/lib/Makefile.displayvar EPICS_SITE_TOP)

if [ ! -d ${EPICS_SITE_TOP} ]; then
	echo "EPICS tools directory ${EPICS_SITE_TOP} does not exist."
	return 2
fi

TOOLS_DIR=$(make -f ${EPICS_SITE_TOP}/tools/${TOOLS_MODULE_VERSION}/lib/Makefile.displayvar TOOLS)

if [ ! -d ${TOOLS_DIR} ]; then
	echo "PCDS tools directory ${TOOLS_DIR} does not exist."
	return 3
fi

load_epics_config_var EPICS_BASE
load_epics_config_var EPICS_EXTENSIONS

EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch.pl)
if [ ! -d ${EPICS_BASE}/bin/${EPICS_HOST_ARCH} ]; then
	if [ "X${EPICS_HOST_ARCH}" == "Xlinux-x86_64" ]; then
		# Try behaving as a 32bits system
		EPICS_HOST_ARCH="linux-x86"
	else
		echo "WARNING: could not find binaries built for platform ${EPICS_HOST_ARCH}."
	fi
fi

# Set path to utilities provided by EPICS and its extensions
PATH="${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${TOOLS_DIR}/bin"
PATH="${PATH}:${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}"

# Set path to libraries provided by EPICS and its extensions (required by EPICS tools)
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_BASE}/lib/${EPICS_HOST_ARCH}"
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_EXTENSIONS}/lib/${EPICS_HOST_ARCH}"

# Source all usefull variables out of EPICS_SITE_CONFIG
load_epics_config_var EPICSIOC_VAR_DIR
