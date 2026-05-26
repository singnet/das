#include "MorkWrapper.h"

#include "MapperFactory.h"
#include "MorkConnection.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

MorkWrapper::MorkWrapper(shared_ptr<MorkConnection> conn,
                         shared_ptr<BoundedSharedQueue> output_queue,
                         MAPPER_TYPE mapper_type)
    : DatabaseWrapper(*conn, MapperFactory::create(mapper_type)),
      conn(conn),
      output_queue(output_queue) {}

MorkWrapper::~MorkWrapper() {}

void MorkWrapper::map(const string& metta_query) {
    vector<string> metta_expressions = this->conn->query(metta_query);

    if (metta_expressions.empty()) {
        RAISE_ERROR("No results returned from MORK query: " + metta_query);
    }

    for (const auto& expr : metta_expressions) {
        LOG_DEBUG("Mapping MORK expression: " << expr);

        vector<shared_ptr<Atom>> atoms;

        try {
            MettaExpression metta_expr{expr};
            atoms = this->mapper->map(DbInput{metta_expr});
            LOG_DEBUG("Mapped " << atoms.size() << " atoms from MORK expression");
        } catch (const exception& e) {
            RAISE_ERROR("Error mapping MORK expression: " + expr + " : " + string(e.what()));
            return;
        }

        if (atoms.empty()) {
            RAISE_ERROR("No atoms mapped from MORK expression: " + expr);
        }

        std::queue<shared_ptr<Atom>>* batch_queue = new std::queue<shared_ptr<Atom>>();

        for (const auto& atom : atoms) {
            LOG_INFO("Insert into the queue: " << atom->to_string());
            batch_queue->push(atom);
        }

        unique_lock<mutex> lock(this->api_mutex);
        this->output_queue->enqueue((void*) batch_queue);
    }
}
