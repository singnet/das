#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "SystemParameters.h"
#include "SystemParametersSingleton.h"

namespace das_test {

/** JSON fixture aligned with agents.*.params sections in config/das.json. */
extern const char kAgentsJson[];

/** Build a SystemParameters instance from {@link kAgentsJson}. */
commons::SystemParameters make_test_parameters();

/** Install test parameters into {@link commons::SystemParametersSingleton}. */
void init_test_system_parameters_singleton();

}  // namespace das_test
