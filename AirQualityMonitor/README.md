# ![EnvironmentalSensor](https://i.imgur.com/e4Gf8NV.png)

Firmware for ESP32 boards with many sensors for **Air Quality Monitoring**.

![Air Quality Monitor UIs](https://i.imgur.com/.png)

### Capabilities

Sensors:
* CJMCU-8128
* Plantower PMS7003

Works with:
* ESP Dash (v4) web interface
* ESP Dash (v3) web interface ([customizations](espdash/README.md))
* Bluetooth Low Energy API ([documentation](doc/geigercounter-ble-api.md))

### Dependencies

- Arduino IDE
- ESP32 platform plugin (v1 or v2)
- ESPDash (v3 or v4)
  - AsyncTCP
  - Arduino JSON
- Button2 library


## Instructions

### Hardware

TODO

### ESP32 settings

* 80 MHz CPU and 40 MHz flash frequencies are fine, use more if you want a (bit) more responsive web interface
* 2 MB APP and 2 MB FATFS partition scheme is required (needs more than 1 MB for the app)

### Firmware

Just build and upload the AirQualityMonitor.ino sketch to your ESP32.

Change `WIFI_SSID` and `WIFI_PASSWD` if you want the ESP32 to connect to your local WiFi network. Otherwise it will create its own access point.  
You can then access the embeded web server using the actual IP address, or with the following URL: `http://AirQualityMonitor.local` (but ONLY if your computer/device is compatible with mDNS).


## Get involved!

### Developers

You can browse the code on the GitHub page, submit patches and pull requests! Your help would be greatly appreciated ;-)

### Users

You can help us find and report bugs, suggest new features, help with translation, documentation and more! Visit the Issues section of the GitHub page to start!


## License

MIT License

> Copyright (c) 2020 Emeric Grange <emeric.grange@gmail.com>
