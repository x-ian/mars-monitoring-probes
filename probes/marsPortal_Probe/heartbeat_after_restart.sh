#!/bin/bash

sleep 30

PATH=$PATH:/bin:/usr/bin
source $(dirname "$0")/counter_restarts.txt

((RESTART_COUNTER++))
echo "RESTART_COUNTER=$RESTART_COUNTER" > $(dirname "$0")/counter_restarts.txt

$(dirname "$0")/heartbeat.sh restart
