#include "BodyTracking.h"
#include <ctime>

BodyTracking::BodyTracking(): time(std::time(0)) {}

std::string BodyTracking::JSONOut() {
	std::string json;
	json = "{\"timestamp\": " + std::to_string(time) + ", "
		"\"bodies\": [";

	for (BodyPosition &i : bodies) {
		json += i.JSONOut();
		json += ",";
	}

	json.pop_back();
	json += "]}";

	return json;
}

