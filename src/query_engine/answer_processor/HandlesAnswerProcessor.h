#pragma once

#include "HandlesAnswer.h"
#include "QueryAnswerProcessor.h"
#include "QueryNode.h"

using namespace std;
using namespace query_node;

namespace query_engine {

class HandlesAnswerProcessor : public QueryAnswerProcessor {
   public:
    HandlesAnswerProcessor(const string& local_id, const string& remote_id)
        : output_buffer(make_unique<QueryNodeClient<HandlesAnswer>>(
              local_id, remote_id, MessageBrokerType::GRPC)) {}
    virtual ~HandlesAnswerProcessor() { this->graceful_shutdown(); };
    virtual void process_answer(QueryAnswer* query_answer) override {
        HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
        if (handles_answer) {
            this->output_buffer->add_query_answer(handles_answer);
        }
    }
    virtual void query_answers_finished() override { this->output_buffer->query_answers_finished(); }
    virtual void graceful_shutdown() override { this->output_buffer->graceful_shutdown(); }

    virtual bool is_work_done() override { return this->output_buffer->is_work_done(); }

   protected:
    unique_ptr<QueryNodeClient<HandlesAnswer>> output_buffer;
};

}  // namespace query_engine
