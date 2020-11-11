
<img src="lungs-solid.svg" width="320px" alt="GeigerCounter" align="right" />

## About the ESP32 Air Quality Monitor

* The [Air Quality Monitor]() is meant to keep you alive by monitoring your environment
* It has sensors to relay temperature, humidity, light intensity, particule levels, CO2 levels, VOC levels and much more...
* Uses Bluetooth Low Energy (BLE) and has a limited range

## Features

* Read real-time sensor values
* Features can be extended through available GPIO and an open firmware

## Protocol

The device uses BLE GATT for communication.  
Sensor values are immediately available for reading, through standard read/notify characteristics.  

### BLE & GATT

The basic technologies behind the sensors communication are [Bluetooth Low Energy (BLE)](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy) and [GATT](https://www.bluetooth.com/specifications/gatt).
They allow the devices and the app to share data in a defined manner and define the way you can discover the devices and their services.
In general you have to know about services and characteristics to talk to a BLE device.

### Services, characteristics and handles

The name advertised by the device is `AirMonitor`

##### Generic Access (UUID 00001800-0000-1000-8000-00805f9b34fb)

| Characteristic UUID                  | Handle | Access      | Description |
| ------------------------------------ | ------ | ----------- | ----------- |
| 00002a00-0000-1000-8000-00805f9b34fb | 0x16   | read        | device name |

##### Battery service (UUID 0000180f-0000-1000-8000-00805f9b34fb)

| Characteristic UUID                  | Handle | Access      | Description               |
| ------------------------------------ | ------ | ----------- | ------------------------- |
| 00002a19-0000-1000-8000-00805f9b34fb | 0x44   | read        | battery level             |

##### Data service (UUID eeee9a32-a000-4cbd-b00b-6b519bf2780f)

| Characteristic UUID                  | Handle | Access      | Description                            |
| ------------------------------------ | ------ | ----------- | -------------------------------------- |
| eeee9a32-a001-4cbd-b00b-6b519bf2780f | 0x2a   | read        | get device name                        |
| eeee9a32-a002-4cbd-b00b-6b519bf2780f | 0x2c   | read        | get firmware version                   |
| eeee9a32-a003-4cbd-b00b-6b519bf2780f | 0x2e   | read        | get battery level                      |
| eeee9a32-a0a0-4cbd-b00b-6b519bf2780f | 0x30   | read/notify | get HiGrow realtime data               |
| eeee9a32-a0b0-4cbd-b00b-6b519bf2780f | 0x0?   | read/notify | get Air Monitor realtime data          |
| eeee9a32-a0c0-4cbd-b00b-6b519bf2780f | 0x0?   | read/notify | get Geiger Counter realtime data       |

<img src="endianness.png" width="400px" alt="Endianness" align="right" />

### Data structure

The data is encoded on bytes in little-endian.  
This means that the data is represented with the least significant byte first.

To understand multi-byte integer representation, you can read the [endianness](https://en.wikipedia.org/wiki/Endianness) Wikipedia page.

### Name

A read request to the `0x16` handle will return n bytes of data, for example `0x4169724d6f6e69746f72` corresponding to the device name.

| Position | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 |
| -------- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Value    | 41 | 69 | 72 | 4d | 6f | 6e | 69 | 74 | 6f | 72 |

| Bytes | Type       | Value      | Description |
| ----- | ---------- | ---------- | ----------- |
| all   | ASCII text | AirMonitor | device name |

### Firmware

A read request to the `0x2c` handle will return 3 bytes of data, for example `0x302e33`.

| Position | 00 | 01 | 02 |
| -------- | -- | -- | -- |
| Value    | 30 | 2e | 31 |

| Bytes | Type       | Value | Description        |
| ----- | ---------- | ----- | ------------------ |
| all   | ASCII text | 0.3   | firmware version   |

##### Battery service (UUID 0000180f-0000-1000-8000-00805f9b34fb)

| Characteristic UUID                  | Handle | Access      | Description               |
| ------------------------------------ | ------ | ----------- | ------------------------- |
| 00002a19-0000-1000-8000-00805f9b34fb | 0x44   | read        | battery level             |

### Real-time data

A read request will return 16 bytes of data, for example `0x00`.  
You can subscribe to this handle and and receive notifications for new values (once per second) by writing 2 bytes (`0x0100`) to the Client Characteristic Configuration descriptor (`0x2902`).  

| Position | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
| -------- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Value    | 40 | 01 | 25 | 15 | 00 | 00 | 64 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 |

| Bytes | Type       | Value | Description                |
| ----- | ---------- | ----- | -------------------------- |
| 00-15 | -          | -     | reserved                   |

### Advertisement data

None

## Reference

[1] https://emeric.io/EnvironmentalSensors/  
[2] https://github.com/emericg/esp32-environmental-sensors/tree/master/AirMonitor  

## License

MIT
