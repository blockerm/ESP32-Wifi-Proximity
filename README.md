# ESP32-Wifi-Proximity
This is an ESP32 project to detect proximity of MAC addresses via WIFI and initiate a NodeRed message for Home/Away status.

I had a problem with my kids leaving their lights on when leaving the house.  I use HomeSeer for lighting control, so I added the MQTT plug in which enabled me to turn off the lights if they were detected as away.  The code sniffs the network in promiscuous mode to identify MAC addresses.  The addresses are compared to a known list of addresses to detect and publish status. 

I leveraged some existing code and wanted to share the result.  It has been 100% reliable so far.  In large part this was due to restarting the ESP32 every few hours.  When restarted, the status for all known MACs defaults to "START."  Because HomeSeer is only looking for HOME/AWAY status, this initial status is ignored, and the correct status is re-established after the initial time-out period (default is 900 seconds).

The project leverages the EspMQTTClient library.
