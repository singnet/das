#ifndef _ATTENTION_BROKER_SERVER_TESTS_TESTUTILS
#define _ATTENTION_BROKER_SERVER_TESTS_TESTUTILS

#include <string>
#include <math.h>

using namespace std;

double random_importance();
string random_handle();
string sequential_label(unsigned int &count, string prefix = "v");
string prefixed_random_handle(string prefix);
string *build_handle_space(unsigned int size, bool sort=false);
bool double_equals(double v1, double v2);

#endif
