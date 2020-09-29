# M5Stack Sonoff Power Display
A display for Sonoff-Tasmota POW R2 to show voltage, current, power and energy values.
It includes also an alarm function to create an alarm tone in case of power loss.

![Display1](https://github.com/AK-Homberger/M5Stack-Sonoff-Power-Display/blob/master/IMG_1278.JPG)

![Display2](https://github.com/AK-Homberger/M5Stack-Sonoff-Power-Display/blob/master/pow-r2-04_2.jpg)

To use the program on the M5Stack you have to install a Sonoff POW R2 device on the boat external power connection.

The Tasmota firmware have to be flashed on the device (I'm using version 7.2). The flashing procedure is detailled here: https://www.tasmota.info/sonoff-pow-r2-flash/ and here: https://github.com/arendst/Tasmota/wiki/sonoff-pow-r2

The M5Stack requires standard installation of the M5Stack libraries. In addition the "ArduinoJson" library has to be installed (from now on version 6.x.x is supported).

The M5Stack acts as a WLAN AccessPoint. Set the ssid and password to your preffered values. The same values have to be used in the configuration web page of the Sonoff POW R2 device.

The Sonoff device connects as wifi client to the M5Stack. The M5Stack receives the power data from Sonoff device via JSON. 
Normally clients get the IP address via DHCP. But it is more reliable to set the Sonoff device to a fixed IP address (192.168.4.100).

To change the IP address you have to access the M5Stack Access Point with a phone/tablet and access the Sonoff device via 192.168.4.2. Make sure that the Sonoff device connects first and then the phone/tablet. Otherwise the IP adresses assigend via DHCP might be different. Just try 192.168.4.2, 192.168.4.3, 192.168.4.4.

When connected to the Tasmota configuration page, open the console and put in the following commands:

- IPAddress 192.168.4.100
- PowerOnState 1
- WattRes 1

Then restart the Sonoff device. It should connect now to the M5Stack and power values should be shown on the device.
"PowerOnState 1" means that the relay of the Sonoff POW is always on after restart. "WattRes 1" means one digit resolution for power value.

You can use the three buttons on the M5Stack for:

- Button A: To togggle the alarm state (alarm in case of power loss).
- Button B: To reset the energy (in kWh) to zero.
- Button C: To set the dimm level of the LCD screen.

The values for alarm state/dimm level are stored in nonvolatile memory in the M5Stack and reloaded after restart.
If the M5Stack can't connect to a Sonoff POW device it will restart after 10 tries. 

# Alternative Display
The M5Stack might be a bit oversized to act only as a display. 
Therfore there is also an alternative solution with an ESP8266 (Wemos D1 mini) and a cheap SSD1306 OLED display:
https://github.com/AK-Homberger/M5Stack-Sonoff-Power-Display/tree/master/ESP8266Power_OLED

A nice housing for the display can be found on Thingiverse: http://www.thingiverse.com/thing:3080488


# Updates
25.07.2020, Version 0.2: Moved to new ArduinoJSON library (version 6.x.x).
