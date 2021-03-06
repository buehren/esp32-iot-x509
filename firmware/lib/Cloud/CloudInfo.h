#ifndef BASECLOUD_H
#define BASECLOUD_H

#include "Config.h"
#include "Utilities.h"
#include "CloudMisc.h"
#include "BaseCloudProvider.h"

#define ms_TO_S_FACTOR 1000    /* Conversion factor for milliseconds to seconds */
class CloudInfoClass : public BaseConfigInfoClass
{
public:
    CloudInfoClass();

    void begin(SemaphoreHandle_t flag);
    bool connect(DATABUILDER builder, DESIREDPROCESSOR processor);
    void load(JsonObjectConst obj) override;
    void save(JsonObject ob) override;
    void toJson(JsonObject ob) override;
    void tick();
    BaseCloudProvider* getProvider();

private:
    static const char* getStringFromProviderType(CloudProviderType type);
    static CloudProviderType getProviderTypeFromString(const char* type);    
    static void loadCertificate(CERTIFICATE *cert);

    IOTCONFIG _config;
    BaseCloudProvider *_provider;
    // Need this so we can save the JSON correctly
    char ca_azure_fileName[32];
    char ca_aws_fileName[32];
};

extern CloudInfoClass CloudInfo;

// "endpoint": "",
// "port": 8883,
// "sendTelemetry": true,
// "intervalSeconds": 45

#endif