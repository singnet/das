#include <iostream>
#include <string>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "Link.h"
#include "Node.h"
#include "RedisMongoDB.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

void load_animals_data();
