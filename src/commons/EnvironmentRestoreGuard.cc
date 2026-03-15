#include "EnvironmentRestoreGuard.h"

#include <cstdlib>

using namespace std;

namespace commons {

void EnvironmentRestoreGuard::save_and_apply(const map<string, string>& overrides) {
    for (const auto& [key, value] : overrides) {
        const char* prev = getenv(key.c_str());
        saved_[key] = prev ? string(prev) : string();
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void EnvironmentRestoreGuard::restore() {
    for (const auto& [k, v] : saved_) {
        setenv(k.c_str(), v.c_str(), 1);
    }
    saved_.clear();
}

}  // namespace commons
