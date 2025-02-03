#pragma once

#include "CountAnswer.h"
#include "QueryAnswerProcessor.h"
#include "QueryNode.h"

using namespace std;
using namespace query_node;

namespace query_engine {

class CountAnswerProcessor : public QueryAnswerProcessor {
   public:
    CountAnswerProcessor(const string& local_id, const string& remote_id)
        : count(0),
          output_buffer(make_unique<QueryNodeClient>(local_id, remote_id, MessageBrokerType::GRPC)) {}
    virtual ~CountAnswerProcessor() { this->graceful_shutdown(); };
    virtual void process_answer(QueryAnswer* query_answer) override {
        if (query_answer) count++;
    }
    virtual void query_answers_finished() override {
        this->output_buffer->add_query_answer(new CountAnswer(count));
        this->output_buffer->query_answers_finished();
    }
    virtual void graceful_shutdown() override { this->output_buffer->graceful_shutdown(); }

   protected:
    int count;
    unique_ptr<QueryNodeClient> output_buffer;
};

}  // namespace query_engine
