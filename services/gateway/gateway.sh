#!/bin/sh
# init.d style script to start and stop a gateway
# Syntax is $0 start | stop <cas_subnet_name>

CAS_SUBNET_NAME=$2

if [ -z "${EPICS_SITE_CONFIG}" ]; then
	EPICS_SITE_CONFIG=/reg/g/pcds/package/epics/RELEASE_SITE
fi

if [ ! -e ${EPICS_SITE_CONFIG} ]; then
	echo "ERROR: cannot find ${EPICS_SITE_CONFIG}."
	exit 1
fi

if [ ! -r $(dirname ${EPICS_SITE_CONFIG})/tools/current/bin/iocenv.sh ]; then
	echo "ERROR: cannot source $(dirname ${EPICS_SITE_CONFIG})/tools/current/bin/iocenv.sh."
	exit 5
fi

# Source the environment for Linux based IOCs
source $(dirname ${EPICS_SITE_CONFIG})/tools/current/bin/iocenv.sh

# Look up the Gateway installation path
load_epics_config_var GATEWAY

# Source the environment
# This will load gateway.env, then gateway.<area>.env
# etc until gateway.<subnet>.<area>.env
LIST=""
NAME=${CAS_SUBNET_NAME}
while [ -n "${NAME}" ]; do
	LIST="${NAME} ${LIST}"
	NAME=$(echo ${NAME} | sed -e 's|[^\.]\+[\.]*||')
done

if [ -e ${GATEWAY}/etc/default.gwenv ]; then
	echo "Loading ${GATEWAY}/etc/default.gwenv"
	source ${GATEWAY}/etc/default.gwenv
fi
for net in ${LIST}; do
	if [ -e ${GATEWAY}/etc/${net}.gwenv ]; then
		echo "Loading ${GATEWAY}/etc/${net}.gwenv"
		source ${GATEWAY}/etc/${net}.gwenv
	fi
done

# Basic check of the configuration
if [ -z "${CAS_IPADDR}" ]; then
	echo "ERROR: no CA Server IP for ${CAS_SUBNET_NAME}, must exit."
	exit 5
fi
# Calculate all IPs of the computer on which the gateway is running
# We use this variable to tell teh gateway to ignore any client
# relayed by another gateway running on this computer
OTHER_GATEWAY_IPS=
for ip in $(/sbin/ifconfig | grep 'inet addr:' | sed -e 's|^.*inet addr:\([^[:space:]]*\).*$|\1|' | grep -v ${CAS_IPADDR}); do
	OTHER_GATEWAY_IPS="${OTHER_GATEWAY_IPS} ${ip}"
done
OTHER_GATEWAY_IPS=${OTHER_GATEWAY_IPS:1}

case $1 in
	"start")
		# Setup run-time paths
		if [ ! -d ${OUT_DIR} ]; then
			mkdir -p ${OUT_DIR}
			if [ $? -ne 0 ]; then
				echo "ERROR: could not create log directory ${OUT_DIR}."
				exit 10
			fi
		fi
		if [ ! -d ${HOME_DIR} ]; then
			mkdir -p ${HOME_DIR}
			if [ $? -ne 0 ]; then
				echo "ERROR: could not create run-time directory ${HOME_DIR}."
				exit 11
			fi
		fi
		if [ -e ${CMD_FILE} ]; then
			rm ${CMD_FILE}
			if [ $? -ne 0 ]; then
				echo "ERROR: could not remove old command file ${CMD_FILE}."
				exit 12
			fi
		fi
		# Build the command line
		PARAMS="-pvlist ${CFG_DIR}/${CAS_SUBNET_NAME}.pvlist"
		PARAMS="${PARAMS} -access ${CFG_DIR}/${CAS_SUBNET_NAME}.access"
		PARAMS="${PARAMS} -log ${LOG_FILE}"
		PARAMS="${PARAMS} -command ${CMD_FILE}"
		PARAMS="${PARAMS} -home ${HOME_DIR}"
		PARAMS="${PARAMS} -sip ${CAS_IPADDR}"
		if [ -n "${CAS_PORT}" ]; then
			PARAMS="${PARAMS} -sport ${CAS_PORT}"
		fi
		if [ -n "${CAC_IPADDR}" ]; then
			PARAMS="${PARAMS} -cip ${CAC_IPADDR}"
		fi
		if [ -n "${CAC_PORT}" ]; then
			PARAMS="${PARAMS} -cport ${CAC_PORT}"
		fi
		PARAMS="${PARAMS} -prefix ${PV_PREFIX}"
		if [ -n "${CONNECT_TIMEOUT}" ]; then
			PARAMS="${PARAMS} -connect_timeout ${CONNECT_TIMEOUT}"
		fi
		if [ -n "${INACTIVE_TIMEOUT}" ]; then
			PARAMS="${PARAMS} -inactive_timeout ${INACTIVE_TIMEOUT}"
		fi
		if [ -n "${DEAD_TIMEOUT}" ]; then
			PARAMS="${PARAMS} -dead_timeout ${DEAD_TIMEOUT}"
		fi
		if [ -n "${DISCONNECT_TIMEOUT}" ]; then
			PARAMS="${PARAMS} -disconnect_timeout ${DISCONNECT_TIMEOUT}"
		fi
		if [ -n "${RECONNECT_INHIBIT}" ]; then
			PARAMS="${PARAMS} -reconnect_inhibit"
		fi
		if [ -n "${DISABLE_CACHE}" ]; then
			PARAMS="${PARAMS} -no_cache"
		fi
		if [ -n "${OTHER_GATEWAY_IPS}" ]; then
			PARAMS="${PARAMS} -signore \"${OTHER_GATEWAY_IPS}\""
		fi
		PARAMS="${PARAMS} -server"
		${EPICS_EXTENSIONS}/bin/${EPICS_HOST_ARCH}/gateway ${PARAMS}
		if [ $? -ne 0 ]; then
			echo "ERROR: could not start gateway."
			exit 15
		fi
		;;
	"stop")
		if [ -e ${HOME_DIR}/gateway.killer ]; then
			source ${HOME_DIR}/gateway.killer
			sleep 1
			rm ${HOME_DIR}/gateway.killer
		else
			echo "No gateway running for ${CAS_SUBNET_NAME} on $(hostname)."
			exit 50
		fi
		if [ -e ${CMD_FILE} ]; then
			rm ${CMD_FILE}
		fi
		;;
	*)
		echo "Invalid command $1."
		exit 99
esac
exit 0
