#pragma once

#include <memory>
#include <vector>

#include "Atom.h"
#include "DatabaseMapper.h"
#include "DatabaseTypes.h"
#include "MettaMapping.h"
#include "MettaParserActions.h"

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

class Metta2AtomsMapper : public DatabaseMapper {
   public:
    Metta2AtomsMapper();
    ~Metta2AtomsMapper() override;

    const vector<Atom*> map(const DbInput& data) override;

   private:
    vector<Atom*> atoms;
    shared_ptr<MettaParserActions> parser_actions;
};

}  // namespace db_adapter