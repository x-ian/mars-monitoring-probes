#!/bin/bash

# ./create_message.sh heartbeat customer_id 2 1 2 20121022-135502 11 22 33 44
#./create_message.sh heartbeat customer_id 2 1 2 20121022-135502 11 22 33 44

MESSAGE_TYPE_ID=$1
CUSTOMER_ID=$2
PROBE_ID=$3
OUTGOING_MESSAGE_COUNT=$4
RESTART_COUNT=$5
DEVICE_TIME=$6
VALUE1=${7}
VALUE2=${8}
VALUE3=${9}
VALUE4=${10}

#URL=http://localhost:3000/messages/create_from_probe
URL=http://www.marsmonitoring.com/messages/create_from_probe

VALUES=\"message\":{\"data\":\"$MESSAGE_TYPE_ID,$CUSTOMER_ID,$PROBE_ID,$OUTGOING_MESSAGE_COUNT,$RESTART_COUNT,$DEVICE_TIME,$VALUE1,$VALUE2,$VALUE3,$VALUE4\"}

#VALUES=\"message\":{\"data\":\"PAYLOAD,1,38,103,114,20111225-001412,14,16468,,\"}
#VALUES=\"message\":{\"data\":\"PAYLOAD,2,5,103,114,20111225-001412,14,16468,,\"}

#curl -v -H "Accept: application/json" -H "Content-type: application/json" -X POST -d " {`echo $VALUES`}" $URL
wget --timeout=5 --tries=1 -v --header "Accept: application/json" --header "Content-type: application/json" --post-data="{`echo $VALUES`}" $URL
