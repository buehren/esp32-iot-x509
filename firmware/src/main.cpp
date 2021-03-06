#include <Arduino.h>

#include <SPIFFS.h>

#include "LogInfo.h"
#include "Display.h"
#include "DeviceInfo.h"
#include "WiFiInfo.h"
#include "NTPInfo.h"
#include "GpsInfo.h"
#include "LedInfo.h"
#include "EnvSensor.h"
#include "CloudInfo.h"

SemaphoreHandle_t xSemaphore;

/**
 * Build the data object that will be sent to the cloud
 * 
 * @param payload This is the json object that will contain the data
 * @param isDeviceTwin Device Twin requires different payload
*/
void buildDataObject(JsonObject payload, bool isDeviceTwin)
{
    payload.clear();
    WiFiInfo.toJson(payload);
    LedInfo.toJson(payload);
    EnvSensor.toJson(payload);
    // Currently Azure Device Twin object cannot accept JSON array object which GeoJSON format uses, therefore
    // we send GeoJSON to the telemetry topic but a none array object to Device Twin topic
    if (isDeviceTwin)
    {
        GpsSensor.toJson(payload);
    }
    else
    {
        GpsSensor.toGeoJson(payload);
    }
}

/**
 * This is the desired state received from the cloud.
 * 
 * @param payload This is the desired json object
 */
void updateConfig(JsonObject payload)
{
    auto doc = DynamicJsonDocument(200);
    if (payload.containsKey("device"))
    {
        if (payload["device"].containsKey("location"))
        {
            LogInfo.log(LOG_VERBOSE, F("Found Location Change"));
            if (DeviceInfo.setLocation(payload["device"]["location"].as<char *>()))
            {
                auto loc = doc.createNestedObject("device");
                loc["location"] = payload["device"]["location"].as<char *>();
                CloudInfo.getProvider()->updateProperty(doc.as<JsonObjectConst>());
            }
        }
    }       
    if (payload.containsKey("ledInfo"))
    {
        if (payload["ledInfo"].containsKey("brightness"))
        {
            LogInfo.log(LOG_VERBOSE, F("Found Brightness Change"));
            if (LedInfo.setBrightness(payload["ledInfo"]["brightness"].as<int>()))
            {
                doc.clear();
                auto bright = doc.createNestedObject("ledInfo");
                bright["brightness"] = payload["ledInfo"]["brightness"].as<int>();
                CloudInfo.getProvider()->updateProperty(doc.as<JsonObjectConst>());
            }
        }
    }      
}

/**
 * This is the device setup routine, only called the once on startup
 */
void setup()
{
    Serial.begin(115200);
    xSemaphore = xSemaphoreCreateMutex();
    LogInfo.begin();
    OledDisplay.begin();
    DeviceInfo.begin();
    WiFiInfo.begin();
    EnvSensor.begin(xSemaphore);
    GpsSensor.begin(xSemaphore);
    LedInfo.begin();
    CloudInfo.begin(xSemaphore);

    if (!SPIFFS.begin(true))
    {
        OledDisplay.displayExit(F("An Error has occurred while mounting SPIFFS"));
    }

    Configuration.begin("/config.json");
    Configuration.add(&LogInfo);
    Configuration.add(&LedInfo);
    Configuration.add(&DeviceInfo);
    Configuration.add(&EnvSensor);
    Configuration.add(&GpsSensor);
    Configuration.add(&CloudInfo);
    Configuration.load();
    if (heap_caps_check_integrity_all(true) == false)
    {
        LogInfo.log(LOG_ERROR, F("Heap Corruption detected! -Setup -1"));
        OledDisplay.displayExit(F("Heap Corruption detected! Rebooting"), 5);
    }
    LedInfo.switchOn(LED_POWER);
    LedInfo.blinkOn(LED_POWER);
    OledDisplay.clear();
    OledDisplay.displayLine(0, 10, "ID : %s", DeviceInfo.getDeviceId());
    OledDisplay.displayLine(0, 20, "Loc: %s", DeviceInfo.getLocation());
    LogInfo.log(LOG_VERBOSE, "Connecting to sensors");
    EnvSensor.connect();
    GpsSensor.connect();

    WiFiInfo.connect(0, 30);

    if (WiFiInfo.getIsConnected())
    {
        OledDisplay.displayLine(0, 40, "Tim: %s", NTPInfo.getFormattedTime());
        OledDisplay.displayLine(0, 50, "Env: %s", EnvSensor.toString());
        OledDisplay.displayLine(0, 60, "GPS: %s", GpsSensor.toString());
        // if (CloudInfo.connect(buildDataObject, updateConfig) == false)
        // {
        //     OledDisplay.displayExit(F("Not Connected to the cloud so rebooting to try again!"), 20);
        // }
        if (heap_caps_check_integrity_all(true) == false)
        {
            LogInfo.log(LOG_ERROR, F("Heap Corruption detected! -setup -5"));
            OledDisplay.displayExit(F("Heap Corruption detected! Rebooting"), 5);
        }        
        LogInfo.log(LOG_VERBOSE, "Startup Completed at %s", NTPInfo.getISO8601Formatted().c_str());
    }
    else
    {
        OledDisplay.displayExit(F("Not Connected to WiFi so rebooting as it pointless continuing!"), 30);
    }
    LedInfo.blinkOff(LED_POWER);
    if (heap_caps_check_integrity_all(true) == false)
    {
        LogInfo.log(LOG_ERROR, F("Heap Corruption detected! -setup -end"));
        OledDisplay.displayExit(F("Heap Corruption detected! Rebooting"), 5);
    }
}

/**
 * This loop function is called every time it exits by the main Arduino bookstrap code.
 */
void loop()
{
    if (WiFiInfo.getIsConnected())
    {
        if (heap_caps_check_integrity_all(true) == false)
        {
            LogInfo.log(LOG_ERROR, F("Heap Corruption detected! -1"));
            OledDisplay.displayExit(F("Heap Corruption detected! Rebooting"), 5);
        }
        OledDisplay.displayLine(30, 50, "%s", NTPInfo.getFormattedTime());
        // EnvSensor.tick();
        // GpsSensor.tick();
        delay(500);
        NTPInfo.tick();
        // CloudInfo.tick();
        // WakeUp.tick();
        OledDisplay.displayLine(30, 40, "%s", EnvSensor.toString());
        OledDisplay.displayLine(30, 60, "%s", GpsSensor.toString());
        delay(500);
    }
    else
    {
        OledDisplay.displayExit(F("Not Connected to WiFi so rebooting as it pointless continuing!"), 30);
    }
}