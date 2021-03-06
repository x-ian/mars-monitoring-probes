#!/bin/bash

if [ -z "$1" ]; then
	# if nothing, use heartbeat
	MESSAGE_TYPE=heartbeat
else
	# use first param
	MESSAGE_TYPE=$1
fi

PATH=$PATH:/bin:/usr/bin
BASEDIR=$(dirname `realpath $0`)

source $BASEDIR/config.txt
source $BASEDIR/counter_restarts.txt
source $BASEDIR/counter_outgoing_messages.txt

TIME=$(date +%Y%m%d-%H%M%S)

ping -qc 10 192.168.0.21
PING_1=$?
ping -qc 10 192.168.0.22
PING_2=$?
ping -qc 10 192.168.0.23
PING_3=$?
ping -qc 10 192.168.0.24
PING_4=$?
ping -qc 10 192.168.0.25
PING_5=$?
ping -qc 10 192.168.0.11
PING_6=$?

$BASEDIR/create_message.sh $MESSAGE_TYPE $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $PING_1 $PING_2 $PING_3 $PING_4 $PING_5 $PING_6

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt
