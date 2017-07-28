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

# <td>Credit balance</td>
# <td>MK 0.0</td>
# <td></td>
#
#<td>Data Me2U</td>
#<td>0 MB</td>
#<td>Aug 01 14:00 2017</td>

EXPIRY_DATE=`curl -s http://aoc.mw.airtellive.com/balance | grep -A2 "Data Me2U" | tail -1 | sed 's/<\/td>//' | sed 's/<td>//' | sed 's/MK //' | sed 's/ MB//' | awk '{$1=$1};1'`

DATA=`curl -s http://aoc.mw.airtellive.com/balance | grep -A1 "Data Me2U" | tail -1 | sed 's/<\/td>//' | sed 's/<td>//' | sed 's/MK //' | sed 's/ MB//' | awk '{$1=$1};1'`

BALANCE=`curl -s http://aoc.mw.airtellive.com/balance | grep -A1 "Credit balance" | tail -1 | sed 's/<\/td>//' | sed 's/<td>//' | sed 's/MK //' | sed 's/ MB//' | awk '{$1=$1};1'`


$BASEDIR/create_message.sh $MESSAGE_TYPE $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $BALANCE $DATA $EXPIRY_DATE

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt
