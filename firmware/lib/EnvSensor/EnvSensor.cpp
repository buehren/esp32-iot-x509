#include "LogInfo.h"
#include "EnvSensor.h"
#include "NTPInfo.h"
#include "WakeUpInfo.h"

RTC_DATA_ATTR int _envCount;

/**
 * overridden begin method to initialise the environment sensor and assign the semaphore flag
 * 
 * @param flag The semaphore flag to use
 */
void EnvSensorClass::begin(SemaphoreHandle_t flag)
{
    this->_semaphoreFlag = flag;
    //Check if we are waking up or we have started because of manual reset or power on
    if (WakeUp.isPoweredOn())
    {
        _envCount = 0;
    }

    this->_humidity = 0.0;
    this->_temperature = 0.0;
}

/**
 * overridden load JSON element into the EnvSensor instance
 * 
 * @param json The ArduinoJson object that this element will be loaded from
 */
void EnvSensorClass::load(JsonObjectConst obj)
{
    this->_scale = obj.containsKey("scale") ? static_cast<ScaleType>(obj["scale"].as<int>()) : ENV_CELSIUS;
    this->_dataPin = obj.containsKey("data") ? obj["data"].as<int>() : 14;
    this->_enabled = obj.containsKey("enabled") ? obj["enabled"].as<bool>() : false;
    this->_sampleRate = obj.containsKey("sampleRate") ? obj["sampleRate"].as<int>() : 2500;
    this->_sensor = SimpleDHT22(this->_dataPin);
    LogInfo.log(LOG_VERBOSE, "Env Data: %i Enabled: %s",
                this->_dataPin, this->getIsEnabled() ? "Yes" : "No");
}

/**
 * overridden save JSON element from the EnvSensor instance
 * 
 * @param json The ArduinoJson object that this element will be loaded from
 */
void EnvSensorClass::save(JsonObject obj)
{
    auto json = obj.createNestedObject(this->_sectionName);
    json["scale"] = (int)this->_scale;
    json["data"] = this->_dataPin;
    json["enabled"] = this->_enabled;
    json["sampleRate"] = this->_sampleRate;
}

/**
 * overridden create a JSON element that will show the current EnvSensor telemetry
 * 
 * @param json The ArduinoJson object that this element will be added to.
 */
void EnvSensorClass::toJson(JsonObject ob)
{
    auto json = ob.createNestedObject("EnvSensor");
    json["temperature"] = this->_temperature;
    json["humidity"] = this->_humidity;
    json["read_count"] = _envCount;
    json["last_read"] = this->_last_read;
    json["last_epoch"] = this->_epoch_time;
}

/**
 * overridden task that will be ran every n sample rate
 * 
 * @return True if successfully read the DHT22 sensor
 */
bool EnvSensorClass::taskToRun()
{
    if (this->getIsEnabled())
    {
        int err = this->_sensor.read2(&this->_temperature, &this->_humidity, NULL);
        this->_last_read = millis();
        if (err != SimpleDHTErrSuccess)
        {
            // Run it again, maybe just a timing issue.
            if (this->taskToRun() == false)
            {
                LogInfo.log(LOG_WARNING, "Problem reading %s sensor - 0x%x", this->getName(), err);
                return false;
            }
        }
        LogInfo.log(LOG_VERBOSE, "Temp = %s @ %s", this->toString(), NTPInfo.getISO8601Formatted().c_str());
        _envCount++;
        this->setEpoch();
        return true;
    }
    return false;
}

/**
 * overridden connect to the sensor and see it is working or not
 * 
 * @return True if successfully connect
 */
const bool EnvSensorClass::connect()
{
    if (this->taskToRun())
    {
        if (this->getSingleThreadFlag() == false)
        {
            LogInfo.log(LOG_VERBOSE, "Creating %s Task on Core 0", this->getName());
            xTaskCreatePinnedToCore(EnvSensorClass::task, "ReadEnvTask",
                                    10000,
                                    (void *)&this->_instance,
                                    1,
                                    &this->_taskHandle,
                                    0);
        }
        else
        {
            LogInfo.log(LOG_VERBOSE, "%s Task will run in single thread mode on Core 1", this->getName());
        }
        this->_connected = true;
    }
    LogInfo.log(LOG_VERBOSE, "Env is connected : %s", this->getIsConnected() ? "Yes" : "No");
    return this->_connected;
}

/**
 * get the current scale temperature symbol
 * 
 * @return The symbol
 */
const char *EnvSensorClass::getSymbol()
{
    switch (this->_scale)
    {
    case ENV_CELSIUS:
        return "°C";
    case ENV_FAHRENHEIT:
        return "°F";
    case ENV_KELVIN:
        return "°K";
    };
    return "UNK";
}

/**
 * get display friendly info
 * 
 * @return The buffer pointer
 */
const char *EnvSensorClass::toString()
{
    snprintf(this->_toString, sizeof(this->_toString), "%0.2f%s (%0.2f%%)",
             this->_temperature,
             this->getSymbol(),
             this->_humidity);
    return this->_toString;
}

/**
 * override update the enabled flag and update the configuration has changed flag
 * 
 * @param flag Is enabled true or false now
 */
void EnvSensorClass::changeEnabled(bool flag)
{
    if (flag != this->getIsEnabled())
    {
        this->_enabled = flag;
        this->_changed = true;
    }
}

EnvSensorClass EnvSensor;