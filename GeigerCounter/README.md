# ![EnvironmentalSensor](https://i.imgur.com/e4Gf8NV.png)

Firmware for ESP32 boards with "CAJOE" **Geiger Counter** modules.

The CAJOE module has a [Geiger Müller tube](https://en.wikipedia.org/wiki/Geiger%E2%80%93M%C3%BCller_tube) used for the detection of ionizing radiation.

This board is made for long term ambient radiation monitoring.  
You might not be planning on going to Chernobyl, but Chernobyl might just come to you...

![GeigerCounter UIs](https://i.imgur.com/8hAUht4.png)

### Capabilities

Sensors:
* Event count (detect ionising event due to a radiation particle)
* Event count per second
* Radiation levels (µSv/s)
* Radiation levels (µSv/m)
* Radiation status (OK > careful > you're gonna die)

Works with:
* ESP Dash (v3) web interface ([customizations](espdash/README.md))
* Bluetooth Low Energy API ([documentation](doc/geigercounter-ble-api.md))

### Dependencies

- Arduino IDE
- ESP32 platform plugin
- ESPDash (v3)
  - AsyncTCP
  - Arduino JSON
- Button2 library


## Instructions

### Hardware

Just connect the CAJOE board pins to your ESP32.

| CAJOE | > | ESP32 |
| ----- | - | ----- |
| 5V    | > | 5V    |
| GND   | > | GND   |
| VIN   | > | D13   |

### ESP32 settings

* 80 MHz CPU and 40 MHz flash frequencies are fine
* 2 MB APP and 2 MB FATFS partition scheme is requiered

### Firmware

Just build and upload the GeigerCounter.ino sketch to your ESP32.

Change `WIFI_SSID` and `WIFI_PASSWD` if you want the ESP32 to connect to your local WiFi network. Otherwise it will create its own access point.  
You can then access the embeded web server using the actual IP address, or with the following URL: `http://GeigerCounter.local` (but ONLY if your computer/device is compatible with mDNS).


## Get involved!

### Developers

You can browse the code on the GitHub page, submit patches and pull requests! Your help would be greatly appreciated ;-)

### Users

You can help us find and report bugs, suggest new features, help with translation, documentation and more! Visit the Issues section of the GitHub page to start!


## License

MIT License

> Copyright (c) 2020 Emeric Grange <emeric.grange@gmail.com>
