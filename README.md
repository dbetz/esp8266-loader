# esp8266-loader
Propeller loader using the ESP8266 WiFi module

Build the firmware by opening the esp8266-firmware.ino file in the Arduino IDE.

To build this, create a file called ap.h containing the following lines:

```
const char WiFiSSID[] = "my_ssid";
const char WiFiPSK[] = "my_password";
```

Of course, replace the above strings with your SSID and password.

You also need to run "make" from the top-level directory if you've modified IP_Loader.spin.
Among other things, this creates the file IP_Loader.h from IP_Loader.spin containing the second-stage
loader binary as C initialized data structures that are included by fastproploader.cpp.
