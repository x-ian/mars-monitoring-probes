Use any (Ubuntu-based) Linux to send out restart and heartbeat messages 
to mars-monitoring.com.

Install the following cronjobs:
@reboot /home/mars/mars-monitoring-probes/probes/Ubuntu_Internet_Probe/measure_network_speed_after_restart.sh 
1,16,31,46 * * * * /home/mars/mars-monitoring-probes/probes/Ubuntu_Internet_Probe/measure_network_speed.sh
