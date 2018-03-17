#!/bin/bash

# measure PING and HTTP speed and post to marsmonitoring.com

if [ -z "$1" ]; then
	# if nothing, use heartbeat
	MESSAGE_TYPE=heartbeat
else
	# use first param
	MESSAGE_TYPE=$1
fi

PATH=$PATH:/bin:/usr/bin
source $(dirname "$0")/config.txt
source $(dirname "$0")/counter_restarts.txt
source $(dirname "$0")/counter_outgoing_messages.txt
source /home/marsPortal/config.txt

TIME=$(date +%Y%m%d-%H%M%S)

#PING=$(ping -qc 10 google.com | awk -F/ '{ print $5 }' | tail -1)
#HTTP=$(wget -O /dev/null http://ipv4.download.thinkbroadband.com/10MB.zip 2>&1 | tail -2 | awk  '{ print $3 " " $4 }' | sed 's/[()]//g')
#HTTP_SPEED=$(echo $HTTP | awk '{ print $1 }' | tr ',' '.')
#HTTP_UNIT=$(echo $HTTP | awk '{ print $2 }')
#if [ "$HTTP_UNIT" == "MB/s" ]; then
#	HTTP_SPEED=$(awk 'BEGIN {OFMT="%.2f";print ("'"$HTTP_SPEED"'" * 1000) }')
#fi

NETGATE_ID=`cat /var/db/uniqueid`
WAN_TRAFFIC=1
DEVICES_EVER_REGISTERED=1
DEVICES_LAST_WEEK=1

$(dirname "$0")/create_message.sh $MESSAGE_TYPE $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $SSH_TUNNEL_PORT $NETGATE_ID $WAN_TRAFFIC $DEVICES_EVER_REGISTERED $DEVICES_LAST_WEEK

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt
