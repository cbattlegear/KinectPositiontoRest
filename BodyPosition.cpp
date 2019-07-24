#include "BodyPosition.h"
#include <string>
using namespace std;

string BodyPosition::JSONOut()
{
	return "{\"id\": " + to_string(id) + ", \"x\": " + to_string(x) + ", \"y\": " + to_string(y) + ", \"z\": " + to_string(z) + "}";
}
