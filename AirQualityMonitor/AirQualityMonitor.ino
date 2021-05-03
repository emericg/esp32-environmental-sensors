/*!
 * Firmware for ESP32 boards for Air Quality Monitoring
 *
 * Settings:
 * - 80 MHz CPU and 40 MHz flash frequencies are fine
 * - 2 MB APP and 2 MB FATFS partition scheme is requiered
 *
 * Dependencies:
 * - ESP32 platform plugin
 * - ESPDash (v3)
 *   - AsyncTCP
 *   - Arduino JSON
 * - Button2 library
 *
 * CJMCU-8128 sensor
 * - SparkFun_CCS811
 * - SparkFun_BME280
 * - ClosedCube_HDC1080
 */

#include <algorithm>
#include <iostream>

#include <Arduino.h>
#include <time.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_wifi.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <ESPmDNS.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <Button2.h>
#include <Wire.h>
#include <SparkFunCCS811.h>
#include <SparkFunBME280.h>
#include <ClosedCube_HDC1080.h>

CCS811 sensor_CCS811(0x5A);
ClosedCube_HDC1080 sensor_HDC1080;
BME280 sensor_BME280;

/* ************************************************************************** */

#define FIRMWARE_NAME       "AirQualityMonitor"
#define FIRMWARE_VERSION    "0.4"

#define PIN_BUTTON_BOOT      0
#define PIN_LED              2

/* ************************************************************************** */

AsyncWebServer server(80);
WiFiMulti multi;

#define WIFI_AP_MODE        false
#define WIFI_MDNS_NAME      "AirQualityMonitor"
#define WIFI_SSID           ""
#define WIFI_PASSWD         ""

/* ************************************************************************** */

#define UUID_BLE_INFOS_SERVICE    "0000180a-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_INFOS_DEVICE     "00002a24-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_INFOS_FIRMWARE   "00002a26-0000-1000-8000-00805f9b34fb"

#define UUID_BLE_BATTERY_SERVICE  "0000180f-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_BATTERY_LEVEL    "00002a19-0000-1000-8000-00805f9b34fb"

#define UUID_BLE_DATA_SERVICE     "eeee9a32-a000-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_DATA_REALTIME    "eeee9a32-a0a0-4cbd-b00b-6b519bf2780f"

/* ************************************************************************** */

BLEServer *bleServer = nullptr;

BLEService *bleInfosService = nullptr;
BLECharacteristic *bleInfosDevice = nullptr;
BLECharacteristic *bleInfosFirmware = nullptr;

BLEService *bleBatteryService = nullptr;
BLECharacteristic *bleBatteryLevel = nullptr;

BLEService *bleDataService = nullptr;
BLECharacteristic *bleDataAir_rt = nullptr;
//BLECharacteristic *bleDataAir_VOC = nullptr;
//BLECharacteristic *bleDataAir_eCO2 = nullptr;
//BLECharacteristic *bleDataAir_temp = nullptr;
//BLECharacteristic *bleDataAir_humi = nullptr;
//BLECharacteristic *bleDataAir_pressure = nullptr;

bool bleClientConnected = false;
uint64_t bleTimestamp = 0;
char bleData[16] = {0};

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        //Serial.println("+ bleServer onConnect()");
        bleTimestamp = millis();
        bleClientConnected = true;
    }
    void onDisconnect(BLEServer *pServer) {
        //Serial.println("- bleServer onDisconnect()");
        bleTimestamp = 0;
        bleClientConnected = false;
        pServer->startAdvertising(); // restart advertising
    }
};
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
        // Do something before the read completes
    }
    void onWrite(BLECharacteristic *pCharacteristic) {
        // Do something because a new value was written
    }
};

void ble_setup()
{
    BLEDevice::init(WIFI_MDNS_NAME);
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallbacks());

    // Info service
    bleInfosService = bleServer->createService(UUID_BLE_INFOS_SERVICE);
    bleInfosDevice = bleInfosService->createCharacteristic(UUID_BLE_INFOS_DEVICE, BLECharacteristic::PROPERTY_READ);
    bleInfosDevice->setValue(WIFI_MDNS_NAME);
    bleInfosFirmware = bleInfosService->createCharacteristic(UUID_BLE_INFOS_FIRMWARE, BLECharacteristic::PROPERTY_READ);
    bleInfosFirmware->setValue(FIRMWARE_VERSION);
    bleInfosService->start();

    // Battery service
    // this device doesn't have a battery

    // Data service
    bleDataService = bleServer->createService(UUID_BLE_DATA_SERVICE);

    //bleDataAir_rt->setCallbacks(new CharacteristicCallbacks())
    bleDataAir_rt = bleDataService->createCharacteristic(UUID_BLE_DATA_REALTIME,
                                                         BLECharacteristic::PROPERTY_READ |
                                                         BLECharacteristic::PROPERTY_NOTIFY);
    bleDataAir_rt->setValue("");
    bleDataAir_rt->addDescriptor(new BLE2902());

    bleDataService->start();

    // Start BLE
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(UUID_BLE_DATA_SERVICE);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
}

/* ************************************************************************** */

// NTP
const char *ntpServer = "pool.ntp.org";
const long gmtOffset = +1;
const long gmtOffset_sec = gmtOffset * 60 * 60;
const bool daylightOffset = true;
const int daylightOffset_sec = daylightOffset * 3600;

bool ntpTimeSet = false;

void ntp_setup()
{
    // Get datetime from NTP servers
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Check results
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        ntpTimeSet = true;
        Serial.println("NTP time set");
        Serial.println(&timeinfo, "- %A, %B %d %Y %H:%M:%S");
    }
    else
    {
        ntpTimeSet = false;
        Serial.println("Failed to obtain NTP time...");
    }
}

/* ************************************************************************** */

void wifi_setup()
{
#if (WIFI_AP_MODE == 1)
    Serial.println("WIFI Configuring host access point...");

    char wifi_ap_name[64] = WIFI_MDNS_NAME;
    strcat(wifi_ap_name, " Access Point");
    WiFi.softAP(wifi_ap_name);
#else
    Serial.println("WiFi Connecting to local access point...");

    WiFi.mode(WIFI_STA);
    multi.addAP(WIFI_SSID, WIFI_PASSWD);
    multi.run();

    //wifi_config_t current_conf;
    //esp_wifi_get_config(WIFI_IF_STA, &current_conf);
    //int ssidlen = strlen((char *)(current_conf.sta.ssid));
    //int passlen = strlen((char *)(current_conf.sta.password));
    //
    //if (ssidlen == 0 || passlen == 0) {
    //    multi.addAP(WIFI_SSID, WIFI_PASSWD);
    //    Serial.println("Connect to default SSID");
    //
    //    int timeout = 16;
    //    while (multi.run() != WL_CONNECTED && timeout > 0) {
    //        Serial.print('.');
    //        timeout--;
    //        delay(250);
    //    }
    //} else {
    //    WiFi.begin();
    //}

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Connection failed!");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("- MAC Address: ");
        Serial.println(WiFi.macAddress());
        Serial.print("- IP Address: ");
        Serial.println(WiFi.localIP());

        ntp_setup();
    }
#endif // WIFI_AP_MODE
}

void wifi_disconnect()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void wifi_monitor()
{
    if (multi.run() != WL_CONNECTED)
    {
        Serial.println("WiFi Reconnection...");
        wifi_disconnect();
        delay(500);
        wifi_setup();
        //if (connectioWasAlive == true)
        //{
        //    connectioWasAlive = false;
        //    Serial.print("Looking for WiFi ");
        //}
        //Serial.print(".");
        //delay(250);
    }
    //else if (connectioWasAlive == false)
    //{
    //    connectioWasAlive = true;
    //    Serial.printf(" connected to %s\n", WiFi.SSID().c_str());
    //}

}

/* ************************************************************************** */

ESPDash dashboard(&server);

Card ctemp(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
Card chumi(&dashboard, HUMIDITY_CARD, "Humidity", "%");
Card cpres(&dashboard, GENERIC_CARD, "Pressure");
Card cvoc(&dashboard, GENERIC_CARD, "VOC");
Card cco2(&dashboard, GENERIC_CARD, "CO2");
Card airquality(&dashboard, STATUS_CARD, "Air quality", "idle");

bool httpServerBegin_v3()
{
    static bool isBegin = false;
    if (isBegin) return true;

    if (MDNS.begin(WIFI_MDNS_NAME)) {
        Serial.println("MDNS responder started");
    }
    MDNS.addService("http", "tcp", 80);

    isBegin = true;
    server.begin();

    return true;
}

/* ************************************************************************** */
/* ************************************************************************** */

void setup()
{
    Serial.begin(115200);

    wifi_setup();
    delay(250);

    ble_setup();
    delay(250);

    Wire.begin();

    sensor_BME280.setI2CAddress(0x76);
    sensor_BME280.beginI2C();
    sensor_BME280.setReferencePressure(101200); // Adjust the sea level pressure used for altitude calculations

    sensor_HDC1080.begin(0x40);

    sensor_CCS811.begin();
}

/* ************************************************************************** */

uint64_t timestamp = 0;

float temperature_bme280;
float temperature_hdc1080;
float humidity_bme280;
float humidity_hdc1080;
float pressure_bme280;
float eCO2_ccs811;
float tVOC_ccs811;

void loop()
{
    if (millis() - timestamp > 2500)
    {
        timestamp = millis();
        //Serial.print("timestamp: ");
        //Serial.println(&timestamp, "%i");

        wifi_monitor();
        httpServerBegin_v3();

        ////////

        if (sensor_CCS811.dataAvailable())
        {
            // Have the sensor read and calculate the results
            sensor_CCS811.readAlgorithmResults();

            temperature_bme280 = sensor_BME280.readTempC();
            temperature_hdc1080 = sensor_HDC1080.readTemperature();
            humidity_bme280 = sensor_BME280.readFloatHumidity();
            humidity_hdc1080 = sensor_HDC1080.readHumidity();
            pressure_bme280 = sensor_BME280.readFloatPressure() / 100.f;
            eCO2_ccs811 = sensor_CCS811.getCO2();
            tVOC_ccs811 = sensor_CCS811.getTVOC();

            Serial.print("BME280  Temp["); Serial.print(temperature_bme280, 1);
            Serial.print("C] RH["); Serial.print(humidity_bme280, 1);
            Serial.print("%] Pressure["); Serial.print(pressure_bme280, 0);
            Serial.println("hPa] ");

            //Serial.print("Alt["); Serial.print(sensor_BME280.readFloatAltitudeMeters(), 0);
            //Serial.println("m] ");

            Serial.print("HDC1080 Temp["); Serial.print(temperature_hdc1080, 1);
            Serial.print("C] RH["); Serial.print(humidity_hdc1080, 1);
            Serial.println("%] ");

            Serial.print("CCS811  CO2["); Serial.print(eCO2_ccs811, 0);
            Serial.print("] tVOC["); Serial.print(tVOC_ccs811, 0);
            Serial.println("] ");

            //Serial.print("uptime["); Serial.print(millis()/1000);
            //Serial.println("s] ");

            // compensating the CCS811 with humidity and temperature readings
            sensor_CCS811.setEnvironmentalData(humidity_hdc1080, temperature_hdc1080);
        }

        ////////

        // BLE
        if (bleClientConnected) {
            //Serial.println("bleClientConnected");

            uint16_t t = (uint16_t)temperature_hdc1080*10;
            uint8_t h = (uint8_t)humidity_hdc1080;
            uint16_t p = (uint16_t)pressure_bme280;
            uint16_t v = (uint16_t)tVOC_ccs811;
            uint16_t c = (uint16_t)eCO2_ccs811;
            bleData[0] = (uint8_t)( t        & 0x00FF);
            bleData[1] = (uint8_t)((t >> 8)  & 0x00FF);
            bleData[2] = h;
            bleData[3] = (uint8_t)( p        & 0x00FF);
            bleData[4] = (uint8_t)((p >> 8)  & 0x00FF);
            bleData[5] = (uint8_t)( v        & 0x00FF);
            bleData[6] = (uint8_t)((v >> 8)  & 0x00FF);
            bleData[7] = (uint8_t)( c        & 0x00FF);
            bleData[8] = (uint8_t)((c >> 8)  & 0x00FF);
            bleData[15] = '\0';

            bleDataAir_rt->setValue((uint8_t *)bleData, 16);
            bleDataAir_rt->notify();

            if (millis() - bleTimestamp > 60000)
            {
                Serial.println("- bleClientConnected TIMEOUT");
                bleServer->disconnect(bleServer->getConnId());
            }
        }

        ////////

        // webserver (ESP DASH v3)
        ctemp.update(temperature_hdc1080);
        chumi.update((int)humidity_hdc1080);
        cpres.update((int)pressure_bme280);
        cvoc.update((int)tVOC_ccs811);
        cco2.update((int)eCO2_ccs811);

        if (tVOC_ccs811 > 1000.f || eCO2_ccs811 > 1500.f) airquality.update("danger", "danger");
        else if (tVOC_ccs811 > 500.f || eCO2_ccs811 > 850.f) airquality.update("warning", "warning");
        else airquality.update("good", "success");

        dashboard.sendUpdates();
    }
}

/* ************************************************************************** */
