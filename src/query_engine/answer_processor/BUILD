package(default_visibility = ["//visibility:public"])

cc_library(
  name = "answer_processor_lib",
  includes = ["."],
  deps = [
    ":query_answer_processor",
    ":attention_broker_updater",
    ":count_answer_processor",
    ":handles_answer_processor",
  ],
)


cc_library(
  name = "attention_broker_updater",
  hdrs = ["AttentionBrokerUpdater.h"],
  deps = [
    ":query_answer_processor",
    "//query_engine:handles_answer",
    "//query_engine:atomdb_api_types",
    "//commons:commons_lib",
    "//attention_broker:attention_broker_server_lib",
  ],
)

cc_library(
  name = "query_answer_processor",
  hdrs = ["QueryAnswerProcessor.h"],
  includes = ["."],
  deps = [
    "//query_engine:query_answer",
    "//query_engine:query_node",
  ],
)

cc_library(
  name = "count_answer_processor",
  hdrs = ["CountAnswerProcessor.h"],
  deps = [
    ":query_answer_processor",
    "//query_engine:count_answer",
    "//query_engine:query_node",
  ],
)

cc_library(
  name = "handles_answer_processor",
  hdrs = ["HandlesAnswerProcessor.h"],
  deps = [
    ":query_answer_processor",
    "//query_engine:handles_answer",
    "//query_engine:query_node",
  ],
)
