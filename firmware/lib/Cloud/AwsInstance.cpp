#include "AwsInstance.h"
#include "LogInfo.h"
#include "NTPInfo.h"
#include "WakeUpInfo.h"
#include "LedInfo.h"

RTC_DATA_ATTR int _Aws_count;

/**
 * This the static callback for processing messages return from the IoT broker
*/
void AwsInstanceClass::mqttCallback(char *topic, byte *payload, unsigned int length)
{
    WakeUp.suspendSleep();
    LedInfo.blinkOn(LED_CLOUD);
    Aws.processReply(topic, payload, length);
    LedInfo.blinkOff(LED_CLOUD);
    WakeUp.resumeSleep();
}

/**
 * Aws Instance Constructor
 */
AwsInstanceClass::AwsInstanceClass() : BaseCloudProvider(CPT_AWS)
{
}

/**
 * Connect and initialise the broker.
 * 
 * @return True if connected
 */
bool AwsInstanceClass::connect(const IoTConfig *config)
{
    if (this->_tryConnecting == false)
    {
        this->_tryConnecting = true;
        this->_config = config;
        this->initialiseConnection(AwsInstanceClass::mqttCallback);
        if (this->mqttConnection())
        {
            this->getCurrentStatus();
        }
    }
    return this->getIsConnected();
}

/**
 * Get the current twin status of the device
 * 
 * @return The current status of the device twin
 */
bool AwsInstanceClass::getCurrentStatus()
{
    bool sent = false;
    if (this->getIsConnected())
    {
        char topic[64];
        strcpy(topic, this->_shadowPrefix);
        strcat(topic, "/get");

        LogInfo.log(LOG_VERBOSE, "Getting Current Status to [%s]", topic);
        sent = this->_mqttClient.publish(
            topic,
            nullptr);
    }
    LogInfo.log(LOG_INFO, "Current GET status is %s at %s", sent ? "True" : "False", NTPInfo.getISO8601Formatted().c_str());
    return sent;
}

/**
 * Build the user name to connect to the hub
 */
void AwsInstanceClass::buildUserName(char *userName)
{
    userName = NULL;
}

/**
 * Check the reply received from the MQTT broker
 * 
 * @param topic The topic the message was received from 
 * @param payload The body of the message, generally in JSON format
 * @param length The size of the payload
 */
void AwsInstanceClass::processReply(char *topic, byte *payload, unsigned int length)
{
    LogInfo.log(LOG_VERBOSE, "Received Aws Reply from [%s][%u]", topic, length);
    DynamicJsonDocument doc(length * 2);
    bool hasBody = false;
    if (length > 0)
    {
        DeserializationError err = deserializeJson(doc, (char *)payload);
        if (err)
        {
            LogInfo.log(LOG_ERROR, "Invalid payload: %i!!!!", err.code());
            return;
        }
        LogInfo.log(LOG_VERBOSE, F("MQTT Update Message"), doc.as<JsonObject>());
        hasBody = true;
    }
    // check for accepted twin update
    char reply[64];
    strcpy(reply, this->_shadowPrefix);
    strcat(reply, "/update/accepted");
    if (strstr(topic, reply) != NULL)
    {
        LogInfo.log(LOG_VERBOSE, "OK Reply from hub ");
    }
    strcpy(reply, this->_shadowPrefix);
    strcpy(reply, "/get/accepted");
    if (strstr(topic, reply) != NULL && hasBody)
    {
        this->processDesiredStatus(doc["state"].as<JsonObject>());
    }

    strcpy(reply, this->_shadowPrefix);
    strcpy(reply, "/update/delta");
    if (strstr(topic, reply) != NULL && hasBody)
    {
        this->processDesiredStatus(doc["state"].as<JsonObject>());
    }    

    LogInfo.log(LOG_VERBOSE, "Finished Updating - Body: %s",
                hasBody ? "Yes" : "No");
}

/**
 * Process the desired properties and set the configuration elements
 * 
 * @param doc The doc object that contains the desired element.
 */
void AwsInstanceClass::processDesiredStatus(JsonObject doc)
{
    JsonObject element;
    if (doc.containsKey("desired"))
    {
        element = doc["desired"].as<JsonObject>();
    }
    else 
    {
        element = doc;
    }

    if (element.containsKey("location"))
    {
        LogInfo.log(LOG_VERBOSE, F("Found Location Change"));
        if (DeviceInfo.setLocation(element["location"].as<char *>()))
        {
            this->updateProperty("location", element["location"]);
        }
    }    
}

/**
 * Load the topics into the provider
 */
void AwsInstanceClass::loadTopics()
{
    strcpy(this->_shadowPrefix, "$aws/things/");
    strcat(this->_shadowPrefix, DeviceInfo.getDeviceId());
    strcat(this->_shadowPrefix, "/shadow");

    strcpy(this->_topics[0].topic, this->_shadowPrefix);
    strcpy(this->_topics[0].topic, "/update");
    this->_topics[0].type = TT_DEVICETWIN;
    strcpy(this->_topics[1].topic, this->_shadowPrefix);
    strcpy(this->_topics[1].topic, "/update/delta");
    this->_topics[1].type = TT_SUBSCRIBE;
    strcpy(this->_topics[2].topic, this->_shadowPrefix);
    strcpy(this->_topics[2].topic, "/update/accepted");
    this->_topics[2].type = TT_SUBSCRIBE;
    strcpy(this->_topics[3].topic, "devices/");
    strcat(this->_topics[3].topic, DeviceInfo.getDeviceId());
    strcat(this->_topics[3].topic, "/messages/events/");
    this->_topics[3].type = TT_TELEMETRY;
    this->_topicsAdded = 4;
}

AwsInstanceClass Aws;