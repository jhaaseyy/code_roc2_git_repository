#!/bin/sh

# if the deploy.sh file exists and causes OCCULT_DEPLOY to have value TRUE 
# subsequent parts of the startup scripts behave as being in deployment mode
# 

if test -e /etc/init.d/deploy.sh ; then
	. /etc/init.d/deploy.sh
fi

if test -e /etc/init.d/device_address.sh ; then
	. /etc/init.d/device_address.sh
fi

export PATH=/bin/occult:$PATH

export GPS_BAUD_RATE=38400
if test "$OCCULT_DEPLOY" = "TRUE" ; then
	export OCCULT_HOME=/var
	export ARCHIVE_DIRECTORY=$OCCULT_HOME/archive
else 
	export OCCULT_HOME=/mnt/cf
fi
export CONFIG_STATE_FILE=$OCCULT_HOME/occult/config_state
export DEFAULT_CONFIG_STATE_FILE=$OCCULT_HOME/occult/default_config_state
export BASE_TRANSMISSION_DIRECTORY=$OCCULT_HOME/occult/transmit
export PREP_TRANSMISSION_DIRECTORY=$OCCULT_HOME/occult/prep
export DATASET_LOCATION=$OCCULT_HOME/occult/datasets
export QUEUE_STATE_FILE=$OCCULT_HOME/occult/queue_state
export ARBITRATION_CONFIGURATION=$OCCULT_HOME/occult/arbitration

export ARBITRATOR_SOCKET=/tmp/arbitrator
export XMIT_PIPENAME=/tmp/CNESserver
export CNES_PROTOCOL_DEVICE=/dev/ttyAPP1

export LED_RED=/sys/class/gpio/gpio195/value
export LED_GRN=/sys/class/gpio/gpio197/value

