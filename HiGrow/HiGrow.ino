/*!
 * Custom firmware for HiGrow ESP32 boards.
 * Tested with a LilyGO T-HiGrow v1.1
 *
 * Other variants may need adjustments in order to work. Free to contact me
 * if you found such adjustment so we can set them this up properly in here!
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
 * - BH1750 library
 * - DHT library (for DTH11, DTH21, DTH22)
 * - DHT12 library (for DHT12)
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
#include <BH1750.h>
#include <DHT12.h>
#include <DHT.h>

#define FIRMWARE_NAME    "HiGrow"
#define FIRMWARE_VERSION "0.3"

/* ************************************************************************** */
/*
// Pinout for the original board
#define PIN_BUTTON_BOOT      0
#define PIN_BUTTON_WAKE     35
#define PIN_POWER_CTRL      -1
#define PIN_LED             16 // blue LED
#define PIN_DHT             22
#define PIN_I2C_SDA         -1
#define PIN_I2C_SCL         -1
#define PIN_MOISTURE        32
#define PIN_LIGHT_ADC       33
#define PIN_BAT_ADC         34
#define PIN_FERTILITY       -1
*/
// Pinout for the LilyGO variant
#define PIN_BUTTON_BOOT      0
#define PIN_BUTTON_WAKE     35
#define PIN_POWER_CTRL       4
#define PIN_LED             -1
#define PIN_DHT             16
#define PIN_I2C_SDA         25
#define PIN_I2C_SCL         26
#define PIN_MOISTURE        32
#define PIN_LIGHT_ADC       -1
#define PIN_BAT_ADC         33
#define PIN_FERTILITY       34

/* ************************************************************************** */

BH1750 lightMeter(0x23);
DHT dht(PIN_DHT, DHT11);        // For DHT11 / DHT21 / DHT22
//DHT12 dht(PIN_DHT, true);     // For DHT12

Button2 bootButton(PIN_BUTTON_BOOT);
Button2 wakeButton(PIN_BUTTON_WAKE);

/* ************************************************************************** */

AsyncWebServer server(80);
WiFiMulti multi;

#define WIFI_AP_MODE      false
#define WIFI_MDNS_NAME    "HiGrow"
#define WIFI_SSID         ""
#define WIFI_PASSWD       ""

/* ************************************************************************** */

#define UUID_BLE_BATTERY_SERVICE  "0000180f-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_BATTERY_CHAR     "00002a19-0000-1000-8000-00805f9b34fb"

#define UUID_BLE_SERVICE          "eeee9a32-a000-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_INFOS_DEVICE     "eeee9a32-a001-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_INFOS_FIRMWARE   "eeee9a32-a002-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_INFOS_BATTERY    "eeee9a32-a003-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_DATA_HIGROW      "eeee9a32-a0a0-4cbd-b00b-6b519bf2780f"

/* ************************************************************************** */

BLEServer *bleServer = nullptr;
BLEService *bleBatteryService = nullptr;
BLECharacteristic *bleBatteryChar = nullptr;
BLEService *bleDataService = nullptr;
BLECharacteristic *bleInfosDevice = nullptr;
BLECharacteristic *bleInfosFirmware = nullptr;
BLECharacteristic *bleInfosBattery = nullptr;
BLECharacteristic *bleDataHiGrow = nullptr;

bool bleClientConnected = false;
char bleBattery[1] = {100};
char bleData[16] = {0};

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        bleClientConnected = true;
    }
    void onDisconnect(BLEServer *pServer) {
        bleClientConnected = false;
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

    // Battery service
    bleBatteryService = bleServer->createService(UUID_BLE_BATTERY_SERVICE);
    bleBatteryChar = bleBatteryService->createCharacteristic(UUID_BLE_BATTERY_CHAR, BLECharacteristic::PROPERTY_READ);
    bleBatteryChar->setValue((uint8_t *)bleBattery, 1);
    bleBatteryService->start();

    // Data service
    bleDataService = bleServer->createService(UUID_BLE_SERVICE);
    bleInfosDevice = bleDataService->createCharacteristic(UUID_BLE_INFOS_DEVICE, BLECharacteristic::PROPERTY_READ);
    bleInfosDevice->setValue(WIFI_MDNS_NAME);
    bleInfosFirmware = bleDataService->createCharacteristic(UUID_BLE_INFOS_FIRMWARE, BLECharacteristic::PROPERTY_READ);
    bleInfosFirmware->setValue(FIRMWARE_VERSION);
    bleInfosBattery = bleDataService->createCharacteristic(UUID_BLE_INFOS_BATTERY, BLECharacteristic::PROPERTY_READ);
    bleInfosBattery->setValue((uint8_t *)bleBattery, 1);

    //bleDataHiGrow->setCallbacks(new CharacteristicCallbacks())
    bleDataHiGrow = bleDataService->createCharacteristic(UUID_BLE_DATA_HIGROW,
                                                         BLECharacteristic::PROPERTY_READ |
                                                         BLECharacteristic::PROPERTY_NOTIFY);
    bleDataHiGrow->addDescriptor(new BLE2902());
    bleDataHiGrow->setValue("");

    bleDataService->start();

    // Start BLE
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(UUID_BLE_SERVICE);
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

void setup_network_time()
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
    Serial.println("WIFI Connecting to local access point...");

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
        Serial.printf("WiFi connection failed!\n");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("- MAC Address: ");
        Serial.println(WiFi.macAddress());
        Serial.print("- IP Address: ");
        Serial.println(WiFi.localIP());

        setup_network_time();
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
        Serial.println("wifi reconnection...");
        wifi_disconnect();
        delay(250);
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

void smartConfigStart(Button2 &b)
{
    Serial.println("smartConfigStart...");
    WiFi.disconnect();
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone()) {
        Serial.print(".");
        delay(250);
    }
    WiFi.stopSmartConfig();
    Serial.println("\nsmartConfigStop...");

    Serial.print("> Connected to: "); Serial.println(WiFi.SSID());
    Serial.print("> PSW: "); Serial.println(WiFi.psk());
}

void sleepHandler(Button2 &b)
{
    Serial.println("Entering deepsleep...");
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
    delay(1000);
    esp_deep_sleep_start();
}

/* ************************************************************************** */

ESPDash dashboard(&server);

Card ctemp(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
Card clux(&dashboard, GENERIC_CARD, "Luminosity", "lux");
Card cbatt(&dashboard, GENERIC_CARD, "Batterie voltage", "v");
Card chumidity(&dashboard, HUMIDITY_CARD, "Humidity", "%");
Card cmoisture(&dashboard, PROGRESS_CARD, "Soil moisture", "%", 0, 100);
Card cfertility(&dashboard, GENERIC_CARD, "Soil fertility", "µS/cm");

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

void setup()
{
    Serial.begin(115200);

    wifi_setup();
    delay(250);

    ble_setup();
    delay(250);

    bootButton.setLongClickHandler(smartConfigStart);
    wakeButton.setLongClickHandler(sleepHandler);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    dht.begin();

    // Sensor power control pin, use detect must set high
    pinMode(PIN_POWER_CTRL, OUTPUT);
    digitalWrite(PIN_POWER_CTRL, 1);
    delay(1000);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE)) {
        Serial.println(F("BH1750 Advanced begin"));
    } else {
        Serial.println(F("Error initialising BH1750"));
    }
}

/* ************************************************************************** */

int readLight_i2c()
{
    float lux = lightMeter.readLightLevel();
    if (lux < 0.f) lux = 0.f;
    if (lux > 1000000.f) lux = 0.f;
    return (int)lux;
}

int readLight_adc()
{
    int light = map(analogRead(PIN_LIGHT_ADC), 0, 4095, 0, 1023);
    if (light < 0) light = 0;
    if (light > 1023) light = 1023;
    return light;
}

int readMoisture()
{
    // Lilygo doc say it should be [0-4095], but seems to be ~ [1800-3300]
    int soil = map(analogRead(PIN_MOISTURE), 1800, 3300, 100, 0);
    if (soil < 0) soil = 0;
    if (soil > 100) soil = 100;
    return soil;
}

int readFertility()
{
    uint8_t samples = 120;
    uint16_t array[120];
    int humi = 0;

    for (int i = 0; i < samples; i++) {
        array[i] = analogRead(PIN_FERTILITY);
        delay(2);
    }
    std::sort(array, array + samples);
    for (int i = 0; i < samples; i++) {
        if (i == 0 || i == samples - 1) continue;
        humi += array[i];
    }
    humi /= samples - 2;
    return humi;
}

float readBatteryVolt()
{
    int vref = 1100;
    uint16_t volt = analogRead(PIN_BAT_ADC);
    float battery_voltage = (volt / 4095.f) * 2.f * 3.3f * (vref);
    battery_voltage /= 1000.f; // mV to V
    return battery_voltage;
}

int readBatteryPercent()
{
    int vref = 1100;
    uint16_t volt = analogRead(PIN_BAT_ADC);
    float battery_voltage = (volt / 4095.f) * 2.f * 3.3f * (vref);

    int battery_percent = map(battery_voltage, 3000, 3800, 0, 100);
    if (battery_percent < 0) battery_percent = 0;
    if (battery_percent > 100) battery_percent = 100;
    return battery_percent;
}

/* ************************************************************************** */

uint64_t timestamp = 0;
float temp = 1.f;
float humidity = 1.f;

void loop()
{
    bootButton.loop();
    wakeButton.loop();

    if (millis() - timestamp > 1000)
    {
        timestamp = millis();
        //Serial.print("timestamp: ");
        //Serial.println(&timestamp, "%i");

        wifi_monitor();
        httpServerBegin_v3();

        ////////

        float batV = readBatteryVolt();
        bleBattery[0] = (char)readBatteryPercent();

        int lux = readLight_i2c();
        int soilmoisture = readMoisture();
        int soilfertility = readFertility();

        float th = dht.readHumidity(false);
        float tt = dht.readTemperature();
        if (!isnan(tt) && !isnan(th)) {
            temp = tt;
            humidity = th;
            // Compute heat index and dew point in Celsius (isFahreheit = false)
            //float hic12 = dht12.computeHeatIndex(temp, humidity, false);
            //float dpc12 = dht12.dewPoint(temp, humidity, false);
            //Serial.print("Heat index: "); Serial.println(hic12);
            //Serial.print("Dew point:  "); Serial.println(dpc12);
        } else {
            //Serial.println("[DHT] read error");
        }

        ////////
/*
        // Recap
        Serial.println("RECAP 1");
        Serial.print("- battery v:  "); Serial.println(batV);
        Serial.print("- battery %:  "); Serial.println(batP);
        Serial.print("> temp:     "); Serial.println(temp);
        Serial.print("> humidity: "); Serial.println(humidity);
        Serial.print("> light:    "); Serial.println(lux);
        Serial.print("> soil m:   "); Serial.println(soilmoisture);
        Serial.print("> soil f:   "); Serial.println(soilfertility);
*/
        ////////

        // BLE
        if (bleClientConnected) {
            //Serial.println("bleClientConnected");

            bleBatteryChar->setValue((uint8_t *)bleBattery, 1);
            bleInfosBattery->setValue((uint8_t *)bleBattery, 1);

            uint16_t t = (uint16_t)temp*10;
            uint8_t h = (uint8_t)humidity;
            uint32_t l = (uint32_t)lux;
            uint16_t f = (uint16_t)soilfertility;
            bleData[0] = (uint8_t)( t        & 0x00FF);
            bleData[1] = (uint8_t)((t >> 8)  & 0x00FF);
            bleData[2] = h;
            bleData[3] = soilmoisture;
            bleData[4] = (uint8_t)( f        & 0x00FF);
            bleData[5] = (uint8_t)((f >> 8)  & 0x00FF);
            bleData[6] = (uint8_t)( l        & 0x000000FF);
            bleData[7] = (uint8_t)((l >>  8) & 0x000000FF);
            bleData[8] = (uint8_t)((l >> 16) & 0x000000FF);
            bleData[15] = '\0';

            bleDataHiGrow->setValue((uint8_t *)bleData, 16);
            bleDataHiGrow->notify();
/*
            // Recap
            Serial.println("RECAP BLE");
            for (int i = 0; i < 16; i++) Serial.print(bleData[i], HEX);
            Serial.println();
*/
        }

        ////////

        // webserver (v3)
        clux.update(lux);
        cbatt.update(batV);
        ctemp.update(temp);
        chumidity.update((int)humidity);
        cmoisture.update(soilmoisture);
        cfertility.update(soilfertility);

        dashboard.sendUpdates();
    }
}

/* ************************************************************************** */
