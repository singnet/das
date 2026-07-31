#pragma once

#include "Atom.h"
#include "Utils.h"

using namespace commons;

namespace atoms {

/**
 * Strategy for combining an existing stored Atom with an incoming one during add_*.
 *
 * When AtomDB add_* is called with merger == NULL, the incoming Atom replaces any
 * existing one (upsert). When a Merger is provided and the Atom already exists,
 * backends merge into a working copy and persist only when merge() returns true.
 *
 * Return contract: merge() returns true on success and false on failure. When
 * false is returned, backends must not persist the working copy (existing stored
 * state is left unchanged) and must report that add as "" (single add_* return
 * value, or the corresponding index in a batch handles vector). Soft failures do
 * not abort the rest of a batch.
 *
 * Exception contract: if merge() throws (e.g. ThrowIfExistsMerger), that exception
 * propagates to the caller unchanged. Backends must not wrap or replace it, and
 * must not persist the working copy. Earlier atoms in the same batch may already
 * have been applied (no all-or-nothing precheck).
 *
 * Concurrency: merge-enabled adds are a read-modify-write with no cross-thread
 * atomicity. Concurrent merge-enabled adds for the same handle can lose updates.
 * Callers must serialize merge-enabled adds per handle (single-writer assumption).
 */
class Merger {
   public:
    virtual ~Merger() = default;

    /**
     * @brief Merge incoming into existing (in place).
     * @param existing Working copy of the Atom currently stored in the DB (mutated).
     * @param incoming Atom being added (read-only; must not be mutated).
     * @return true on success (backends may persist existing); false on failure
     *         (backends must leave stored state unchanged).
     * @throws Propagated unchanged by AtomDB backends (e.g. ThrowIfExistsMerger).
     */
    virtual bool merge(Atom* existing, const Atom* incoming) const = 0;
};

/**
 * Merger that rejects any add when the Atom already exists by throwing
 * (replaces throw_if_exists=true). Halts the add_* call via exception; earlier
 * items in a batch may already have been applied.
 */
class ThrowIfExistsMerger : public Merger {
   public:
    bool merge(Atom* existing, const Atom* /*incoming*/) const override {
        string atom_type = Atom::is_node(*existing) ? "Node" : "Link";
        RAISE_ERROR(atom_type + " already exists: " + existing->handle());
        return false;
    }

    static const ThrowIfExistsMerger& instance() {
        static ThrowIfExistsMerger inst;
        return inst;
    }
};

/**
 * Merger that skips any add when the Atom already exists (soft reject).
 * merge() returns false so backends leave stored state unchanged, report "",
 * and continue with the rest of a batch.
 */
class SkipIfExistsMerger : public Merger {
   public:
    bool merge(Atom* /*existing*/, const Atom* /*incoming*/) const override { return false; }

    static const SkipIfExistsMerger& instance() {
        static SkipIfExistsMerger inst;
        return inst;
    }
};

}  // namespace atoms
