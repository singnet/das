#pragma once

#include "Atom.h"
#include "Utils.h"

using namespace commons;

namespace atoms {

/**
 * Strategy for combining an existing stored Atom with an incoming one during add_*.
 *
 * When AtomDB add_* is called with merger == nullptr, the incoming Atom replaces any
 * existing one (upsert). When a Merger is provided and the Atom already exists,
 * backends merge into a working copy and commit only on success so a throwing merger
 * cannot leave partially updated stored state.
 *
 * Exception contract: if merge() throws, that exception propagates to the caller
 * unchanged. Backends must not wrap or replace it (so callers can match messages
 * such as ThrowIfExistsMerger's "Node already exists: <handle>").
 *
 * Concurrency: merge-enabled adds are a read-modify-write with no cross-thread
 * atomicity. Concurrent merge-enabled adds for the same handle can lose updates.
 * Callers must serialize merge-enabled adds per handle (single-writer assumption).
 */
class Merger {
   public:
    virtual ~Merger() = default;

    /**
     * @brief Merge incoming into existing (in place), or throw.
     * @param existing Working copy of the Atom currently stored in the DB (mutated).
     * @param incoming Atom being added (read-only; must not be mutated).
     * @throws Propagated unchanged by AtomDB backends on failure.
     */
    virtual void merge(Atom* existing, const Atom* incoming) const = 0;
};

/**
 * Merger that rejects any add when the Atom already exists (replaces throw_if_exists=true).
 */
class ThrowIfExistsMerger : public Merger {
   public:
    void merge(Atom* existing, const Atom* /*incoming*/) const override {
        string atom_type = Atom::is_node(*existing) ? "Node" : "Link";
        RAISE_ERROR(atom_type + " already exists: " + existing->handle());
    }

    static const ThrowIfExistsMerger& instance() {
        static ThrowIfExistsMerger inst;
        return inst;
    }
};

}  // namespace atoms
