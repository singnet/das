#include "gtest/gtest.h"
#include "MessageBroker.h"

using namespace distributed_algorithm_node;

class MessageFactoryTest : public MessageFactory {
    shared_ptr<Message> message_factory(string &command, vector<string> &args) {
        return shared_ptr<Message>{};
    }
};

TEST(MessageBroker, basics) {

    try {
        MessageBroker::factory((MessageBrokerType) -1, NULL, "");
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }

    try {
        shared_ptr<MessageBroker> message_broker_ram =
            MessageBroker::factory(MessageBrokerType::RAM, shared_ptr<MessageFactory>{}, "");
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }

    try {
        shared_ptr<MessageBroker> message_broke_grpc =
            MessageBroker::factory(MessageBrokerType::GRPC, shared_ptr<MessageFactory>{}, "");
        FAIL() << "Expected exception";
    } catch(std::runtime_error const &error) {
    } catch(...) {
        FAIL() << "Expected std::runtime_error";
    }

    shared_ptr<MessageBroker> message_broker_ram =
        MessageBroker::factory(MessageBrokerType::RAM, shared_ptr<MessageFactory>(new MessageFactoryTest()), "");

    shared_ptr<MessageBroker> message_broker_grpc =
        MessageBroker::factory(MessageBrokerType::GRPC, shared_ptr<MessageFactory>(new MessageFactoryTest()), "");
}
