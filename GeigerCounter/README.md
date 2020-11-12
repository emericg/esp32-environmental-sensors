# ![EnvironmentalSensor](https://i.imgur.com/e4Gf8NV.png)

Firmware for ESP32 boards with "CAJOE" **Geiger Counter** modules.

The CAJOE module has a [Geiger Müller tube](https://en.wikipedia.org/wiki/Geiger%E2%80%93M%C3%BCller_tube) used for the detection of ionizing radiation.

This board is made for long term ambient radiation monitoring.  
You might not be planning on going to Chernobyl, but Chernobyl might just come to you...

### Capabilities

Sensors:
* Event count (detect ionising event due to a radiation particle)
* Event count per second
* Radiation levels (µSv/s)
* Radiation levels (µSv/m)
* Radiation status (OK > careful > you're gonna die)

Works with:
* ESP Dash (v3) web interface
* Bluetooth Low Energy API ([documentation](doc/geigercounter-ble-api.md))

### Dependencies

- Arduino IDE
- ESP32 platform plugin
- ESPDash (v3)
  - AsyncTCP
  - Arduino JSON
- Button2 library

### Settings

* 80 MHz CPU and 40 MHz flash frequencies are fine
* 2 MB APP and 2 MB FATFS partition scheme is requiered


## Get involved!

### Developers

You can browse the code on the GitHub page, submit patches and pull requests! Your help would be greatly appreciated ;-)

### Users

You can help us find and report bugs, suggest new features, help with translation, documentation and more! Visit the Issues section of the GitHub page to start!
