
#ifndef JSON_CONFIG_HANDLER_H
#define JSON_CONFIG_HANDLER_H

#include <string>
#include <ArduinoJson.h>
#include "fatfs.h"

class Remora; //forward declaration

class JsonConfigHandler {
private:

	Remora* remoraInstance;
	std::string jsonContent = "";
	const char* filename = "config.txt";
	JsonDocument doc;
	JsonObject thread;
	//bool configError;
	uint8_t loadConfiguration();
	uint8_t readFileContents();
	uint8_t parseJson();

public:
	JsonConfigHandler(Remora* _remora);
	void updateThreadFreq();
	JsonArray getModules();
	JsonObject getModuleConfig(const char* threadName, const char* moduleType);
};
#endif
