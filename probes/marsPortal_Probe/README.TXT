Install the following cronjobs:
@reboot /home/mars-monitoring-probes/probes/marsPortal_Probe/heartbeat_after_restart.sh 
0 * * * * /home/mars-monitoring-probes/probes/marsPortal_Probe/heartbeat.sh
