#include <string>
#include <vector>

#include "DatabaseTypes.h"

using namespace std;
using namespace db_adapter;

class ContextLoader {
   public:
    static vector<TableMapping> load_table_file(const string& file_path);
    static vector<string> load_query_file(const string& file_path);
};