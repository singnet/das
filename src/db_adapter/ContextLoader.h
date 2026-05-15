#include <string>
#include <vector>

#include "DatabaseTypes.h"

using namespace std;
using namespace db_adapter;

class ContextLoader {
   public:
    static vector<string> load_sql_queries(const string& file_path);
    static vector<string> load_metta_queries(const string& file_path);
};