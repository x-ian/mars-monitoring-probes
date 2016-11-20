#!/bin/bash

# measure PING and HTTP speed and post to marsmonitoring.com

PATH=$PATH:/bin:/usr/bin

TIME=$(date +%Y%m%d-%H%M%S)

PING=$(ping -qc 10 google.com | awk -F/ '{ print $4 }' | tail -1)

# wget -O /dev/null http://ipv4.download.thinkbroadband.com/10MB.zip 2>&1 | tail -2 | awk  '{ print $3 " " $4 }' | sed 's/[()]//g'
# todo, check unit and convert if necessary from MB/s to KB/s
HTTP=$(wget -O /dev/null http://ipv4.download.thinkbroadband.com/10MB.zip 2>&1 | tail -2 | awk  '{ print $3 " " $4 }' | sed 's/[()]//g')
HTTP_SPEED=$(echo $HTTP | awk '{ print $1 }')
HTTP_UNIT=$(echo $HTTP | awk '{ print $2 }')
if [ "$HTTP_UNIT" == "MB/s" ]; then
	HTTP_SPEED=$(awk 'BEGIN {OFMT="%.2f";print ("'"$HTTP_SPEED"'" * 1000) }')
fi
			
$(dirname "$0")/create_message.sh heartbeat customer_id 11 1 1 $TIME $PING $HTTP_SPEED 

rm -f create_from_probe*
