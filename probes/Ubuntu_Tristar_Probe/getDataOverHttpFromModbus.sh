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
  *) status="-1" ;;
esac
BAT_CHARGE_STATE=$status
BAT_CHARGE_CURRENT=$(cat bat-charge-current-39)
BAT_OUTPUT_POWER=$(cat bat-output-power-58)

$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $BAT_PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $BAT_VOLTAGE $BAT_CHARGE_STATE $BAT_CHARGE_CURRENT $BAT_OUTPUT_POWER

ARRAY_VOLTAGE=$(cat array-voltage-27)
ARRAY_CURRENT=$(cat array-current-29)
ARRAY_PMAX=$(cat array-pmax-60)
ARRAY_VMP=$(cat array-vmp-61)

$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $PV_PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $ARRAY_VOLTAGE $ARRAY_CURRENT $ARRAY_PMAX $ARRAY_VMP

BAT_TEMP=$(cat temp-bat-37)
HEATSINK_TEMP=$(cat temp-heatsink-35)
BAT_TARGET_VOLTAGE=$(cat bat-target-voltage-51)

$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $TRISTAR_PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $BAT_TEMP $HEATSINK_TEMP $BAT_TARGET_VOLTAGE

COUNTERS_AMPHOURS=$(cat array-pmax-60)
COUNTERS_KWH=$(cat array-vmp-61)
ALARMS=$(cat error-alarms-46)
case $ALARMS in
  "None") status="0" ;;
  *) status=$ALARMS ;;
esac
ALARMS=$status
FAULTS=$(cat error-faults-44)
case $FAULTS in
  "None") status="0" ;;
  *) status=$FAULTS ;;
esac
FAULTS=$status

$BASEDIR/create_message.sh heartbeat $CUSTOMER_ID $TRISTAR_2_PROBE_ID $OUTGOING_MESSAGE_COUNTER $RESTART_COUNTER $TIME $COUNTERS_AMPHOURS $COUNTERS_KWH $ALARMS $FAULTS

rm -f create_from_probe*
((OUTGOING_MESSAGE_COUNTER++))
echo "OUTGOING_MESSAGE_COUNTER=$OUTGOING_MESSAGE_COUNTER" > $(dirname "$0")/counter_outgoing_messages.txt

