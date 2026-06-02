#pragma once

#include <memory>
#include <vector>

#include "Atom.h"
#include "DatabaseMapper.h"
#include "DatabaseTypes.h"
#include "Link.h"
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

    void map(const DbInput& data, std::queue<shared_ptr<Atom>>& output) override;
    void collect_atoms(std::queue<shared_ptr<Atom>>& output,
                       const string& handle,
                       shared_ptr<MettaParserActions> parser_actions);

   private:
    vector<shared_ptr<Atom>> atoms;
    void collect_atoms_recursive(std::queue<shared_ptr<Atom>>& output,
                                 shared_ptr<Link> link,
                                 shared_ptr<MettaParserActions> parser_actions);
};

}  // namespace db_adapter