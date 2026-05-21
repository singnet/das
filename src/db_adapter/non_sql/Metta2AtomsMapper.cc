#include "Metta2AtomsMapper.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

// ==============================
//  Construction / destruction
// ==============================

Metta2AtomsMapper::Metta2AtomsMapper() {
    RAISE_ERROR("Metta2AtomsMapper constructor not implemented yet");
}

Metta2AtomsMapper::~Metta2AtomsMapper() {}

// ==============================
//  Public
// ==============================

const vector<Atom*> Metta2AtomsMapper::map(const DbInput& data) {
    RAISE_ERROR("Metta2AtomsMapper::map() not implemented yet");
    return vector<Atom*>{};
}