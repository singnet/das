#include "benchmark_runner.h"

Runner::Runner(int tid, int iterations) : tid_(tid), iterations_(iterations){};

const size_t Runner::MAX_COUNT = 1000;
const vector<string> Runner::contains_links_query = {"LINK_TEMPLATE",
                                                     "Expression",
                                                     "3",
                                                     "NODE",
                                                     "Symbol",
                                                     "Contains",
                                                     "VARIABLE",
                                                     "sentence",
                                                     "VARIABLE",
                                                     "word"};

const vector<string> Runner::sentence_links_query = {
    "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "sentence"};