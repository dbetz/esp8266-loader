# esp8266-loader
Propeller loader using the ESP8266 WiFi module

Build the firmware by opening the esp8266-firmware/esp8266-firmware.ino file in the Arduino IDE.

Before you can build the firmware you need to create a file called ap.h in the esp8266-firmware
directory containing the following lines:

```
const char WiFiSSID[] = "my_ssid";
const char WiFiPSK[] = "my_password";
```

Of course, replace the above strings with your SSID and password.

You'll need to use the Arduino IDE to build the firmware. Install the IDE using the
instructions here: https://www.arduino.cc/en/Main/Software

Then install the ESP8266 plug-in by adding this line to the "Additional Board Manager URLs"
list in Preferences:

http://arduino.esp8266.com/stable/package_esp8266com_index.json

You also need to run "make" from the top-level directory if you've modified IP_Loader.spin.
Among other things, this creates the file IP_Loader.h from IP_Loader.spin containing the second-stage
loader binary as C initialized data structures that are included by fastproploader.cpp.
