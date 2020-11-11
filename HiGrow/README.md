# ![EnvironmentalSensor](https://i.imgur.com/e4Gf8NV.png)

Custom firmware for **HiGrow ESP32 boards**. Tested with a Lilygo T-HiGrow v1.1

HiGrow is a plant monitoring sensor platform using ESP32 chips. They are available on a couple of Chinese brands like LilyGo or Wemos

Note that I would not recommend an HiGrow board for any serious plant monitoring, but it's OK for tinkering with an ESP32 board with onboard sensors.
However if you already have one, this firmware might be useful!


### Capabilities

Sensors:
* Temperature
* Humidity
* Light intensity (lux)
* Soil moisture
* Soil fertility (via electrical conductivity)
* Battery level

Works with:
* ESP Dash (v3) web interface
* Bluetooth Low Energy API ([documentation](doc/higrow-api.md))

### Dependencies

- Arduino IDE
- ESP32 platform plugin
- ESPDash (v3)
  - AsyncTCP
  - Arduino JSON
- Button2 library
- BH1750 library
- DHT library (for DTH11, DTH21, DTH22)
- DHT12 library (for DHT12)

### Settings

* 80 MHz CPU and 40 MHz flash frequencies are fine
* 2 MB APP and 2 MB FATFS partition scheme is requiered


## Get involved!

### Developers

You can browse the code on the GitHub page, submit patches and pull requests! Your help would be greatly appreciated ;-)

### Users

You can help us find and report bugs, suggest new features, help with translation, documentation and more! Visit the Issues section of the GitHub page to start!
