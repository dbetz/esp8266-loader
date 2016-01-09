# esp8266-loader
Propeller loader using the ESP8266 WiFi module

To build this create a file called ap.h containing the following lines:

const char WiFiSSID[] = "my_ssid";
const char WiFiPSK[] = "my_password";

You also need to run "make" from the top-level directory before trying to build in the ArduinoIDE.
Among other things, this creates the file IP_Loader.h from IP_Loader.spin containing the second-stage
loader binary as C initialized data structures that are included by fastproploader.cpp.

Of course, replace the above strings with your SSID and password.
