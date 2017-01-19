#!/bin/bash

PATH=$PATH:/bin:/usr/bin
BASEDIR=$(dirname `realpath $0`)

source $BASEDIR/config.txt
source $BASEDIR/counter_restarts.txt
source $BASEDIR/counter_outgoing_messages.txt

TIME=$(date +%Y%m%d-%H%M%S)

rm -rf /tmp/tristardata
mkdir /tmp/tristardata
cd /tmp/tristardata

/usr/bin/phantomjs --web-security=false $BASEDIR/getDataOverHttpFromModbus.js

BAT_VOLTAGE=$(cat bat-battery-voltage-38)
BAT_CHARGE_STATE=$(cat bat-charge-state-50)
case $BAT_CHARGE_STATE in
  "Start") status="1" ;;
  "Night Check") status="2" ;;
  "Night") status="3" ;;
  "MPPT") status="4" ;;
  "Absorption") status="5" ;;
  "Float") status="6" ;;
  "Equalize") status="7" ;;
  "Fault") status="10" ;;
  "Disconnect") status="0" ;;
  *) status=$status ;;
esac
BAT_CHARGE_STATE=$status
BAT_CHARGE_CURRENT=$(cat bat-charge-current-39)
BAT_OUTPUT_POWER=$(cat bat-output-power-58)

$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $BAT_VOLTAGE $BAT_CHARGE_STATE $BAT_CHARGE_CURRENT $BAT_OUTPUT_POWER

#$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME 3 6 9 10

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt

