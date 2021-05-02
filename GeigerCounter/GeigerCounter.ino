/*!
 * Firmware for ESP32 boards with "CAJOE" Geiger Counter modules
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

/* ************************************************************************** */

#define FIRMWARE_NAME       "GeigerCounter"
#define FIRMWARE_VERSION    "0.4"

#define PIN_BUTTON_BOOT      0
#define PIN_LED              2
#define PIN_GEIGER          13

/* ************************************************************************** */

AsyncWebServer server(80);
WiFiMulti multi;

#define WIFI_AP_MODE        false
#define WIFI_MDNS_NAME      "GeigerCounter"
#define WIFI_SSID           ""
#define WIFI_PASSWD         ""

/* ************************************************************************** */

#define UUID_BLE_INFOS_SERVICE    "0000180a-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_INFOS_DEVICE     "00002a24-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_INFOS_FIRMWARE   "00002a26-0000-1000-8000-00805f9b34fb"

#define UUID_BLE_BATTERY_SERVICE  "0000180f-0000-1000-8000-00805f9b34fb"
#define UUID_BLE_BATTERY_LEVEL    "00002a19-0000-1000-8000-00805f9b34fb"

#define UUID_BLE_DATA_SERVICE     "eeee9a32-a000-4cbd-b00b-6b519bf2780f"
#define UUID_BLE_DATA_REALTIME    "eeee9a32-a0d0-4cbd-b00b-6b519bf2780f"

/* ************************************************************************** */

BLEServer *bleServer = nullptr;

BLEService *bleInfosService = nullptr;
BLECharacteristic *bleInfosDevice = nullptr;
BLECharacteristic *bleInfosFirmware = nullptr;

BLEService *bleBatteryService = nullptr;
BLECharacteristic *bleBatteryLevel = nullptr;

BLEService *bleDataService = nullptr;
BLECharacteristic *bleDataGeiger_rt = nullptr;

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

    //bleDataGeiger_rt->setCallbacks(new CharacteristicCallbacks())
    bleDataGeiger_rt = bleDataService->createCharacteristic(UUID_BLE_DATA_REALTIME,
                                                            BLECharacteristic::PROPERTY_READ |
                                                            BLECharacteristic::PROPERTY_NOTIFY);
    bleDataGeiger_rt->setValue("");
    bleDataGeiger_rt->addDescriptor(new BLE2902());

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

Card geigercount(&dashboard, GENERIC_CARD, "Geiger count");
Card geigergauge(&dashboard, PROGRESS_CARD, "Counts", "cps", 0, 10);
Card sievert_m(&dashboard, GENERIC_CARD, "µSv/m");
Card sievert_s(&dashboard, GENERIC_CARD, "µSv/s");
Card radioactivity(&dashboard, STATUS_CARD, "Radioactivity levels", "idle");

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

volatile int counts = 0;            // Tube events
unsigned int previousCounts = 0;
unsigned int cpm = 0;               // count per minute

std::vector <int> vvv;
int vvv_line[60] = {0};

// Captures count of events from Geiger counter board
void ISR_impulse()
{
    counts++;
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

    // Geiger counter pin
    pinMode(PIN_GEIGER, INPUT);
    interrupts();
    attachInterrupt(digitalPinToInterrupt(PIN_GEIGER), ISR_impulse, FALLING);
}

/* ************************************************************************** */

uint64_t timestamp = 0;

void loop()
{
    if (millis() - timestamp > 1000)
    {
        timestamp = millis();
        //Serial.print("timestamp: ");
        //Serial.println(&timestamp, "%i");

        wifi_monitor();
        httpServerBegin_v3();

        ////////

        // geiger
        int instantCount = (counts - previousCounts);
        if (vvv.size() >= 60) vvv.erase(vvv.begin());
        vvv.push_back(instantCount);

        int minuteCount = 0;
        for (int i = 0; i < vvv.size() && i < 60; i++) {
            vvv_line[i] = vvv.at(i);
            minuteCount += vvv.at(i);
        }

        float msv_s = instantCount / 120.f;
        float msv_m = minuteCount / 120.f;

        previousCounts = counts;

        ////////

        // BLE
        if (bleClientConnected) {
            //Serial.println("bleClientConnected");
            char temp[8];

            dtostrf(msv_m, 3, 3, temp);
            //Serial.print("- "); Serial.println(temp);
            bleDataGeiger_rt->setValue(temp);
            bleDataGeiger_rt->notify();

            if (millis() - bleTimestamp > 60000)
            {
                Serial.println("- bleClientConnected TIMEOUT");
                bleServer->disconnect(bleServer->getConnId());
            }
        }

        ////////

        // webserver (ESP DASH v3)
        geigercount.update(counts);
        geigergauge.update(instantCount);
        sievert_m.update(msv_m);
        sievert_s.update(msv_s);

        if (msv_m > 10.f) radioactivity.update("danger", "danger");
        else if (msv_m > 1.f) radioactivity.update("warning", "warning");
        else radioactivity.update("good", "success");

        dashboard.sendUpdates();
    }
}

/* ************************************************************************** */
