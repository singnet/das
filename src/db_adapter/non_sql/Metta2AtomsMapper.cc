#include "Metta2AtomsMapper.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

// ==============================
//  Construction / destruction
// ==============================

Metta2AtomsMapper::Metta2AtomsMapper() {}

Metta2AtomsMapper::~Metta2AtomsMapper() {}

// ==============================
//  Public
// ==============================

void Metta2AtomsMapper::map(const DbInput& data, std::queue<shared_ptr<Atom>>& output) { return; }