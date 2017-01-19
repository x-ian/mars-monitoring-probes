#!/bin/bash

TIME=$(date +%Y%m%d-%H%M%S)

rm -rf /tmp/tristardata
cd /tmp/tristardata

/usr/bin/phantomjs --web-security=false $(dirname "$0")/getDataOverHttpFromModbus.js

BAT_VOLTAGE=$(cat bat-battery-voltage-38)
BAT_CHARGE_STATE=-1
BAT_CHARGE_CURRENT=$(cat bat-charge-current-39)
BAT_OUTPUT_POWER=$(cat bat-output-power-58)

$(dirname "$0")/create_message.sh heartbeat $CUSTOMER_ID $PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $BAT_VOLTAGE $BAT_CHARGE_STATE $BAT_CHARGE_CURRENT $BAT_OUTPUT_POWER`

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt

