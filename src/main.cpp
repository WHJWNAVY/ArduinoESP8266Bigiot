/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <bigiot.h>
#include <Ticker.h> // for LED status
#include <time.h>

#include "main.h"

#define ARDUINOJSON_V5132 ((ARDUINOJSON_VERSION_MAJOR) == 5 && (ARDUINOJSON_VERSION_MINOR) == 13 && (ARDUINOJSON_VERSION_REVISION) == 2)

#if !ARDUINOJSON_V5132
#error "No support Arduinojson version ,please use ArduinoJsonV6.14.1 or ArduinoJsonV5.13.2"
#endif

#define DHTPIN D2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11

Ticker ticker;
WiFiClient client;
BIGIOT bigiotClient(client);
DHT dhtSensor(DHTPIN, DHTTYPE);

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#define BIGIOT_UPLOAD_TIMEOUT 10000 // Data update interval
#define DHTSENSOR_READ_TIMEOUT 1000
#define WIFI_CHECK_TIMEOUT 30000

char *curDateTime(void)
{
    time_t tm = 0;
    struct tm *ptm = NULL;
    static char dateStr[64] = {0};

    time(&tm);
    ptm = localtime(&tm);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", ptm);

    return dateStr;
}

bool ledStatus = true;
void ledTurnOn(void)
{
    ledStatus = true;
    digitalWrite(LED_BUILTIN, LOW);
}

void ledTurnOff(void)
{
    ledStatus = false;
    digitalWrite(LED_BUILTIN, HIGH);
}

void ledToggle(void)
{
    ledStatus = !ledStatus;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

float curTemperature;
float curHumidity;
float curHeatIndex;
void dhtSensorRead(void)
{
    float h, t, hic;

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = dhtSensor.readHumidity();
    // Read temperature as Celsius (the default)
    t = dhtSensor.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
    {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    hic = dhtSensor.computeHeatIndex(t, h, false);

    curTemperature = t;
    curHumidity = h;
    curHeatIndex = hic;

    Serial.printf("Humidity: %f%%\n", curHumidity);
    Serial.printf("Temperature: %f°C, %f°C\n", curTemperature, curHeatIndex);
}

void bigiotEventCallback(const int devid, const int comid, const char *comstr, const char *slave)
{
    // You can handle the commands issued by the platform here.
    Serial.printf("Device: %d, Command: %d, %s\n", devid, comid, comstr);

    if (comid == PLAY)
    {
        ledTurnOn();
    }
    else if (comid == STOP)
    {
        ledTurnOff();
    }
    else if (comid == OFFON)
    {
        ledToggle();
    }
    else
    {
        Serial.printf("Unknown command: %d\n", comid);
    }
}

void bigiotDisconnectCallback(BIGIOT &obj)
{
    ledTurnOff();
    // When the device is disconnected to the platform, you can handle your peripherals here
    Serial.printf("Bigiot %s disconnected!\n", obj.deviceName());
}

void bigiotConnectCallback(BIGIOT &obj)
{
    ledTurnOn();
    // When the device is connected to the platform, you can preprocess your peripherals here
    Serial.printf("Bigiot %s connected!\n", obj.deviceName());
}

char celsiusStr[10] = {0};
char humidityStr[10] = {0};
const char *iotDataId[] = {BIGIOT_DATA_TEMP_ID, BIGIOT_DATA_HUMI_ID};
const char *iotDataVal[] = {celsiusStr, humidityStr};
void bigiotDataUpload(void)
{
    dtostrf(curHeatIndex, sizeof(celsiusStr) - 1, 2, celsiusStr);
    dtostrf(curHumidity, sizeof(humidityStr) - 1, 2, humidityStr);

    Serial.printf("Upload humidity: %s%%\n", humidityStr);
    Serial.printf("Upload temperature: %s°C\n", celsiusStr);

    // Multiple data uploads
    if (!bigiotClient.upload(iotDataId, iotDataVal, 2))
    {
        Serial.println("Sensor data upload failed!");
    }
}

// gets called when WiFiManager enters configuration mode
void wifiManagerConfigCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode.");
    Serial.println(WiFi.softAPIP());
    // if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    // entered config mode, make led toggle faster
    ticker.attach(0.2, ledToggle);
}

void httpServerSendHeader(void)
{
    httpServer.sendHeader("Server", HOSTNAME);
    httpServer.sendHeader("Date", curDateTime());
    httpServer.sendHeader("Build", __DATE__ " " __TIME__);
    httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    httpServer.sendHeader("Pragma", "no-cache");
    httpServer.sendHeader("Expires", "-1");
}

void httpServerHandleRoot(void)
{
    httpServerSendHeader();
    httpServer.send(200, WEB_CONTENT_HTML, WEB_HTML_INDEX);
}

void httpServerHandleNotFound(void)
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += httpServer.uri();
    message += "\nMethod: ";
    message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += httpServer.args();
    message += "\n";

    for (uint8_t i = 0; i < httpServer.args(); i++)
    {
        message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
    }

    httpServerSendHeader();
    httpServer.sendHeader("Content-Length", String(message.length()));
    httpServer.send(404, WEB_CONTENT_TEXT, message);
}

void httpServerHandleCgi(void)
{
    if (httpServer.hasArg("plain") == false)
    {
        httpServer.send(500, WEB_CONTENT_HTML, WEB_HTML_INVALID);
        return;
    }
    String request = httpServer.arg("plain");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(request);
    if (!root.success())
    {
        httpServer.send(500, WEB_CONTENT_HTML, WEB_HTML_INVALID);
        return;
    }

    String result;
    DynamicJsonBuffer resBuffer;
    const char *method = (const char *)root["method"];
    if (strcmp(method, "temp_humi_show") == 0)
    {
        // {"error": {"code": 0}, "result": {"temp": 90, "humi": 59}}
        JsonObject &resRoot = resBuffer.parseObject("{\"error\": {\"code\": 0}, \"result\": {\"temp\": 0, \"humi\": 0}}");
        resRoot["result"]["temp"] = curTemperature;
        resRoot["result"]["humi"] = curHumidity;
        resRoot.printTo(result);
        httpServer.send(200, WEB_CONTENT_JSON, result);
        return;
    }
    else if (strcmp(method, "led_status_show") == 0)
    {
        // {"error": {"code": 0}, "result": {"temp": 90, "humi": 59}}
        JsonObject &resRoot = resBuffer.parseObject(WEB_RESULT_STATUS);
        resRoot["result"]["status"] = ledStatus;
        resRoot.printTo(result);
        httpServer.send(200, WEB_CONTENT_JSON, result);
        return;
    }
    else if (strcmp(method, "led_status_set") == 0)
    {
        // {"method":"led_status_set","params":{"status":false}}
        bool status = root["params"]["status"];
        if (status)
        {
            ledTurnOn();
        }
        else
        {
            ledTurnOff();
        }
        httpServer.send(200, WEB_CONTENT_JSON, WEB_RESULT_STATUS);
        return;
    }
    else
    {
        httpServer.send(500, WEB_CONTENT_HTML, WEB_HTML_INVALID);
        return;
    }
}

const char *bigiotDevid = BIGIOT_DEVICE_ID;
const char *bigiotApikey = BIGIOT_API_KEY;
const char *bigiotUsrkey = "";

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("\n\nHello ESP8266 %s!\n\n", ARDUINO_BOARD);
    Serial.printf("Chip Id: 0x%08x\n", ESP.getChipId());
    Serial.printf("Core Version: %s\n", ESP.getCoreVersion().c_str());
    Serial.printf("SDK Version: %s\n", ESP.getSdkVersion());
    Serial.printf("Boot Version: %d\n", ESP.getBootVersion());
    Serial.printf("Boot Mode: %d\n", ESP.getBootMode());
    Serial.printf("Flash Id: 0x%08x\n", ESP.getFlashChipId());
    Serial.printf("Flash Size: %d\n", ESP.getFlashChipSize());
    Serial.printf("Flash Speed: %d\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash Mode: %d\n\n", ESP.getFlashChipMode());

    // set led pin as output
    pinMode(LED_BUILTIN, OUTPUT);
    // start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.5, ledToggle);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    // reset settings - for testing
    // wifiManager.resetSettings();

    // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(wifiManagerConfigCallback);

    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect())
    {
        Serial.println("Failed to connect and hit timeout");
        // reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("WiFi connected... :)");
    ticker.detach();

    ledTurnOn(); // keep LED on

    configTime(TIMEZONE, NTPSERVER1, NTPSERVER2, NTPSERVER3);

    // Web OTA
    MDNS.begin(HOSTNAME);
    httpUpdater.setup(&httpServer);

    httpServer.on("/", httpServerHandleRoot);
    httpServer.on(WEB_CGI_PATH, HTTP_POST, httpServerHandleCgi);
    httpServer.onNotFound(httpServerHandleNotFound);

    httpServer.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Open http://%s/update to upgrade firmware!\n", HOSTNAME);

    dhtSensor.begin();

    // Regist platform command event hander
    bigiotClient.eventAttach(bigiotEventCallback);

    // Regist device disconnect hander
    bigiotClient.disconnectAttack(bigiotDisconnectCallback);

    // Regist device connect hander
    bigiotClient.connectAttack(bigiotConnectCallback);

    // Login to bigiot.net
    if (!bigiotClient.login(bigiotDevid, bigiotApikey, bigiotUsrkey))
    {
        Serial.println("Bigiot login fail!");
        // reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }
}

uint64_t currentTimes = 0;
uint64_t lastUploadTimes = 0;
uint64_t lastReadTimes = 0;
uint64_t lastCheckTimes = 0;
void loop(void)
{
    currentTimes = millis();
    if (WiFi.status() != WL_CONNECTED)
    {
        // Wifi disconnection reconnection mechanism
        if (currentTimes - lastCheckTimes > WIFI_CHECK_TIMEOUT)
        {
            Serial.println("WiFi connection lost. Reconnecting...");
            WiFi.reconnect();
            lastCheckTimes = currentTimes;
        }
        Serial.print(".");
        return;
    }

    if (currentTimes - lastReadTimes > DHTSENSOR_READ_TIMEOUT)
    {
        lastReadTimes = currentTimes;
        dhtSensorRead();
    }

    // Upload sensor data every 10 seconds
    if (currentTimes - lastUploadTimes > BIGIOT_UPLOAD_TIMEOUT)
    {
        lastUploadTimes = currentTimes;
        bigiotDataUpload();
    }

    MDNS.update();
    httpServer.handleClient();
    bigiotClient.handle();
}