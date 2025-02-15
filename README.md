# (Un)original Prusa Heated Drybox - Port and Modification for Arduino Nano ESP32

This project is intended to be used with the [(Un)original Prusa Heated Drybox](https://www.printables.com/model/883817-unoriginal-prusa-heated-drybox) but

* with an [Arduino Nano ESP32](https://docs.arduino.cc/hardware/nano-esp32/)
* a single rotary button encoder
  * short press outside of menu toggles LEDs
  * long press enters menu or confirms menu entries
  * short press in menu toggles menu pages
* three separate MOSFET for heater, fans, and LEDs
* three additional DS18B20 temperature sensors, one in the duct and two at the front of the drybox
* MQTT and Home Assistant autodiscovery

## Disclaimer

* _The code is still barely tested and might still undergo changes._
* _Unsupervised use of electric heater might be dangerous. Use at your own risk._
