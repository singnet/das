#ifndef _COMMONS_ENVIRONMENT_RESTORE_GUARD_H
#define _COMMONS_ENVIRONMENT_RESTORE_GUARD_H

#include <map>
#include <string>

namespace commons {

/**
 * Saves current environment variables for the given keys, applies overrides
 * via setenv(), and restores the original values in restore() or the destructor.
 * Use when constructing objects that read configuration from the environment,
 * so that overrides do not leak to the rest of the process.
 */
class EnvironmentRestoreGuard {
   public:
    ~EnvironmentRestoreGuard() { restore(); }

    /**
     * For each key in overrides: save current getenv(key), then setenv(key, value, 1).
     */
    void save_and_apply(const std::map<std::string, std::string>& overrides);

    /**
     * Restore all saved environment variable values and clear saved state.
     */
    void restore();

   private:
    std::map<std::string, std::string> saved_;
};

}  // namespace commons

#endif
