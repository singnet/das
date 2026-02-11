#include <string>
#include <vector>

#include "DataTypes.h"

using namespace std;
using namespace db_adapter;

class ContextLoader {
   public:
    static vector<TableMapping> load_context_file(const string& file_path);
};