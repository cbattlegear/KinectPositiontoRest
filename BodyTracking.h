#pragma once
#include <list>
#include <ctime>
#include <string>
#include "BodyPosition.h"

class BodyTracking
{
private:
	std::time_t time;
public:
	std::list<BodyPosition> bodies;
	std::string JSONOut();
	BodyTracking();
};

