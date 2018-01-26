An Arduino with an additional Ethernet Shield (or directly an Arduino 
Ethernet) can be used to create a mars-monitoring compliant Internet
Probe. This is enough to run a simple ping (and maybe also get the 
ping response time), but this is not enough to get a realistic
'Enduser perspective' of the current Internet speed. For this a more 
powerful system is needed.

It turns out that a TP-Link WR703N is a small and cheap Wireless Access
Point which can be flashed with OpenWRT (in fact it is almost required
as the original firmware only comes in a Chinese flavor...).

With this a small, simple and cheap wireless Internet probe can be created.
After flashing with OpenWRT it needs to be hooked into any available Wifi 
and the scheduled job to invoke an HTTP access to measure 'real life HTTP 
performance' does the rest.

http://wiki.openwrt.org/toh/tp-link/tl-wr703n 
http://cmikavac.net/2012/06/03/how-to-flash-tp-link-wr703n-with-openwrt/

Only minor configs to the factory default are necessary if the TP-LINK
should be hooked up wirelessly into the network.
- Resetting (only works ~10 seconds after starting the TP-LINK) and connect
  a LAN cable (http://wiki.openwrt.org/doc/howto/generic.failsafe,
  http://wiki.villagetelco.org/OpenWrt_Failsafe_Mode_and_Flash_Recovery).
- Change default IP address of LAN (192.168.1.1) if it conflicts with your 
  WiFi network.
- Scan for the Wifi and add as WAN interface
- Open remote HTTP access through WAN interface 
  (Network - Firewall - Port forwards and forward port external port 80
  to LAN IP address of OpenWrt with internal port 80)
- Open remote SSH access through WAN interface similar to above remote HTTP
  access with port 22
- Run something like nmap -sP 192.168.1.0/24 in the WiFi network to find
  out which IP address was assigned to TP-LINK
- opkg update
  opkg install bash
  opkg install wget-nossl -force-overwrite 
    (as per http://gregklein.wordpress.com/tag/openwrt/)
- Set proper timezone under System - System
- Copy/paste measure_network_speed.sh and create_message.sh into /root/mars
- Create cronjob with
  # run this script every hour
  01 * * * * /root/mars/measure_network_speed.sh > /dev/null

Use 192.168.99.1 as IP for LAN access with root/root. This interface should have DHCP enabled.

Change /etc/config/wireless with updated WiFi settings; run scan from Web-UI first to detect all available networks.

Allow remote SSH access (via WAN) with this addition to /etc/config/firewall:
config rule
	option target 'ACCEPT'
	option src 'wan'
	option proto 'tcp'
	option family 'ipv4'
	option name 'dropbear ssh WAN'
	option dest_port '22' #or whatever port dropbear is listening

