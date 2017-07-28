#!/bin/bash

sleep 30

# measure PING and HTTP speed and post to marsmonitoring.com

PATH=$PATH:/bin:/usr/bin
source $(dirname "$0")/counter_restarts.txt

((RESTART_COUNTER++))
echo "RESTART_COUNTER=$RESTART_COUNTER" > $(dirname "$0")/counter_restarts.txt

$(dirname "$0")/queryAirtime.sh restart
