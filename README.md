# Portable Wi-Fi Repeater

This is a battery-powered Wi-Fi Repeater based on the ESP8266 board, than can be used to extend the range of a Wi-Fi Network in areas where it could be difficult to use standard Wi-Fi repeaters.

The repeater connects to the Wi-Fi Network to be extended and creates its own Wi-Fi Network. Traffic on the new Wi-Fi Network is routed through the original network.

A solar panel could be used to increase the battery life (18650 Li-ion 3.7V battery). When the battery level is low, the built-in led blinks 3 times every 15 seconds.



## Configuration Mode

When the device starts for the first time, it runs in *Configuration Mode*. An open Wi-Fi Network with SSID *PortWiFiRepeater-nnnn* (where *nnnn* is a 4-digits random number) is created.
Connecting to this network the configuration panel is shown automatically, to configure the Wi-Fi Network to extend and, optionally, the parameters of the network to create.
In the case the configuration panel is not shown automatically, open any browser and go to *http://192.168.1.1*
On the OLED display (if available) some info about the device status are shown.

The configuration panel shows the list of available Wi-Fi Networks, with details about their signal quality. The network to be extended can be selected from the list, and the PSK entered.
By default, the device creates a new Wi-Fi Network with the same SSID with the suffix "_ext", and same PSK.
Both the SSID and PSK of the new network can be customized clicking on the checkbox.
Clicking on the *Refresh* button, a new scan of the available Wi-Fi Networks is performed and the dropdown list is updated accordingly.
Clicking on the *Identify device* button, the built-in led blinks 20 times, to identify the device being configured.

When the configuration is complete, clicking on the *Save* button the parameters are saved in the local EEPROM. The led will blink fast 10 times to inform the configuration has been saved.
Rebooting the device, it will run in *Repeater Mode*.



### Configuration reset

To reset the configuration, press the button and keep it pressed when booting the device.
When the configuration has been reset the led will blink fast 10 times. If available, the OLED will show a message to inform the configuration has been removed.
After a configuration reset, the device can be configured again.



## Repeater Mode

When the device finds a valid configuration in the EEPROM during boot, it starts in *Repeater Mode*.
It first tries to connect to the original Wi-Fi Network, then it creates the new network.
Once the startup process is completed, Wi-Fi devices can connect to the new Wi-Fi Network, and their traffic will be routed automatically through the original network.
The OLED display will show some info about the original network signal quality, the number of connected stations, the battery status and the SSID of the original and new networks.
If the OLED display is not available, the signal quality of the original Wi-Fi Network can be checked pressing the button: the led will blink fast twice (to tell the signal quality will be shown), then after 1 second it will blink slow a number of times equal to the quality of the signal (0 to 5). For example, if the signal quality is 3, the led will blink twice fast, then after 1 second three times slow.

In the case the connection to the original network is lost, the device tried automatically to reconnect. On the OLED display, the *Reconnecting* message is shown.



## Credits and references

Created by [WizLab.it](https://www.wizlab.it/)

GitHub: [https://github.com/wizlab-it/temperature-monitor/](https://github.com/wizlab-it/temperature-monitor/)

[ESP8266 Documentation](https://arduino-esp8266.readthedocs.io/)



## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

To get a copy of the GNU General Public License, see [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/)