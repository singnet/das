#include <iostream>
#include <string>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "Link.h"
#include "Node.h"
#include "RedisMongoDB.h"

using namespace std;
using namespace atomdb;

void load_animals_data();

// Same Similarity and Inheritance facts as load_animals_data(), plus a Related
// Expression for every pair of those facts that share a concept target.
// Concept targets are the handles after the predicate (targets[0] is skipped).
void load_animals_related_data(AtomDB& db);
