# ![EnvironmentalSensor](https://i.imgur.com/e4Gf8NV.png)

Custom firmware for **HiGrow ESP32 boards**.

HiGrow is a plant monitoring sensor platform using ESP32 chips.
Original creator is Lucas Fabbri on [hackaday](https://hackaday.io/project/25253-higrow-plants-monitoring-sensor) and [github](https://github.com/lucafabbri/HiGrow-Arduino-Esp). Boards are still available on a couple of Chinese brands like LilyGO or Wemos

Note that I would not recommend an HiGrow board for any serious plant monitoring, but it's OK for tinkering with an ESP32 board with onboard sensors.
However if you already have one, this firmware might be useful!

![HiGrow UIs](https://i.imgur.com/gfpOMOl.png)

### Capabilities

Sensors:
* Temperature
* Humidity
* Light intensity (lux)
* Soil moisture
* Soil fertility (via electrical conductivity)
* Battery level

Works with:
* ESP Dash (v4) web interface
* ESP Dash (v3) web interface ([customizations](espdash/v3/README.md))
* Bluetooth Low Energy API ([documentation](doc/higrow-ble-api.md))

### Dependencies

- Arduino IDE
- ESP32 platform plugin (v1 or v2)
- ESPDash (v3 or v4)
  - AsyncTCP
  - Arduino JSON
- Button2 library
- BH1750 library
- DHT library (for DTH11, DTH21, DTH22)
- DHT12 library (for DHT12)


## Instructions

### ESP32 settings

* 80 MHz CPU and 40 MHz flash frequencies are fine, use more if you want a (bit) more responsive web interface
* 2 MB APP and 2 MB FATFS partition scheme is required (needs more than 1 MB for the app)

### Firmware

Just build and upload the HiGrow.ino sketch to your ESP32.

Change `WIFI_SSID` and `WIFI_PASSWD` if you want the ESP32 to connect to your local WiFi network. Otherwise it will create its own access point.  
You can then access the embeded web server using the actual IP address, or with the following URL: `http://HiGrow.local` (but ONLY if your computer/device is compatible with mDNS).


## Get involved!

### Developers

You can browse the code on the GitHub page, submit patches and pull requests! Your help would be greatly appreciated ;-)

### Users

You can help us find and report bugs, suggest new features, help with translation, documentation and more! Visit the Issues section of the GitHub page to start!


## License

MIT License

> Copyright (c) 2020 Emeric Grange <emeric.grange@gmail.com>
