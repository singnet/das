# DAS coding standards (singnet/das)

These guidelines reflect how this repository is actually written. Automated reviewers and contributors should follow them instead of generic style guides.

## Project context

- **DAS** (Distributed Atomspace) extends OpenCog Hyperon with a query API backed by cognitive components (Query Engine, Attention Broker, Evolution/Link Creation/Inference agents).
- **Build**: Bazel under `src/`. CI runs `//:format.check` then unit tests on PRs to `develop` and `master`.
- **Architecture**: Service-oriented design around a **Service Bus**. Client code uses `BusCommandProxy` subclasses; server code uses `BusCommandProcessor` subclasses.

## General principles

1. **Minimize scope** — Keep PRs focused. Do not refactor unrelated code.
2. **Match neighbors** — Read surrounding files in the same package before adding code.
3. **Self-explanatory code** — Add comments only for non-obvious business logic or architecture, not for obvious statements.
4. **No drive-by changes** — Do not "modernize" or rename things outside the task.

## C++ (`src/`)

### Formatting (enforced by CI)

- `.clang-format` at repo root and `src/.clang-format`: **4 spaces**, **no tabs**, **105-column** limit, **attach braces**, left pointer alignment (`string* p`).
- Run format check: `cd src && ./scripts/bazel_exec.sh run //:format.check`
- Use `// clang-format off` / `on` only for intentional exceptions (e.g. large token tables).

### Headers and namespaces

- Use `#pragma once`.
- Declare types inside nested namespaces (`namespace agents { ... }`) and close with `}  // namespace agents`.
- **File-level** `using namespace std` and domain `using namespace` lines are standard here. Do not suggest removing them.

### Source file layout

- `.cc` files group methods with section banners:

```cpp
// -------------------------------------------------------------------------------------------------
// Client-side API
```

- Public API in `.h` files uses brief Doxygen `/** ... */` blocks above methods.

### Naming and members

- Classes: `PascalCase`. Methods and fields: `snake_case`.
- Access members with `this->field` consistently.
- Mutex guard pattern: `lock_guard<mutex> semaphore(this->api_mutex);`

### Error handling and logging

- Fatal invariant violations: `RAISE_ERROR("message")` (from `commons/Utils.h`) — logs with stack trace and throws.
- Logging: set `#define LOG_LEVEL INFO_LEVEL` (or `DEBUG_LEVEL`) **before** `#include "Logger.h"`, then use `LOG_DEBUG`, `LOG_INFO`, `LOG_ERROR`.
- In proxy processor threads, catch `std::exception` and report via `proxy->raise_error_on_peer(...)`.

### Patterns to preserve

- **Proxy / processor** split for Service Bus commands.
- `Properties` for command parameters; `tokenize` / `untokenize` for wire format.
- `shared_ptr`, `make_shared`, and explicit `new` appear in existing code — follow the local file's style.
- Static command string constants on proxy classes (e.g. `BaseProxy::ABORT`, `FINISHED`).

### Tests

- C++ unit tests: `src/tests/cpp/*_test.cc`, built with Bazel.
- Add tests for real behavior, not trivial coverage.

## Python (`3rd_party_slots/python_client/`)

- Package: `hyperon_das`. Python **3.10+**.
- **Black**: line length 100, `skip-string-normalization = true` (`.lint/.black.cfg`).
- **isort**: line length 100, trailing commas (`.lint/.isort.cfg`).
- **flake8**: max line length 100; ignore E203, E501 (`.lint/.flake8.cfg`).
- Mirror the existing proxy/client hierarchy and gRPC patterns.
- Use `hyperon_das.logger` for logging.

## Rust (`3rd_party_slots/rust_metta_bus_client/`)

- `rustfmt.toml`: **hard tabs**, `max_width = 100`, `use_small_heuristics = "Max"`.
- Do not reformat to spaces or different width.

## Configuration (`config/`)

- `config/das.json` drives runtime parameters. Preserve key names and structure; document breaking changes.

## What reviewers should **not** suggest

- Removing `using namespace std` or adding redundant `std::` prefixes.
- Google-style or LLVM brace changes conflicting with `.clang-format`.
- Replacing `RAISE_ERROR` with a different error-handling scheme.
- Large refactors, new abstraction layers, or "cleanup" outside the PR scope.
- Comments that merely restate the code.
- Style nits already covered by `//:format.check` or Black/isort.
