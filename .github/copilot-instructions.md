# copilot-instructions for wav-utils-cpp

Purpose: help AI coding agents be immediately productive in this repository.

Quick context
- This repository targets a header-only C++ utility library for .wav files (see `README.md`).
- Canonical reference material lives in `wav-resources/` (notably `WAVE File Format.html`; README also references `riff-specs.pdf` pages 56-65).

What you must know before editing
- The project goal is a header-only library + tutorial-style example code and unit tests (see `README.md`). Treat that as a design constraint: prefer headers and self-contained examples.
- There is currently no build system or tests checked in. Do not add a new dependency or build system without asking the repo owner first.

Conventions and patterns to follow
- Header-only library layout: put public headers under `include/` (e.g. `include/wav/Reader.hpp`). Use `#pragma once` and clear `wav` namespace.
- Source files: avoid `.cpp` files unless implementing optional examples; if you must add them, include straightforward build/run instructions in the PR description.
- Style: prefer explicit, tutorial-style comments. New public API should be concise and documented inline so readers can follow the tutorial flow.
- Testing: add tests under `tests/` and prefer a single-file, header-only test framework (e.g., Catch2 single header) to keep the repo simple. When adding tests include an invocation example in the PR.

Integration & resources
- Use `wav-resources/WAVE File Format.html` as the primary parsed-format reference. When implementing parsing logic, cite the exact section or byte-range in code comments (for example: "See wav-resources/WAVE File Format.html — fmt chunk description").
- Assume little-endian sample storage per the WAVE spec; when in doubt, reference the `riff`/`fmt` chunk details in the resource files.

Build / test guidance (examples — confirm before committing)
- Quick single-file test compile (no CMake):
  - `g++ -std=c++17 -Iinclude tests/test_example.cpp -o tests/test_example && ./tests/test_example`
- If you add a CMakeLists.txt, include a minimal top-level example and a `README.md` change describing `cmake . && make` usage.

When to ask the user
- If you plan to add external dependencies (Catch2, Boost, etc.), a build system, or convert to a multi-file library, ask which approach they prefer.
- If you need to introduce large scaffolding (CI, GitHub Actions), confirm desired CI matrix and test commands first.

PR / commit guidance
- Commit message: imperative, short scope — e.g. `Add wav::Reader header and basic fmt parsing`.
- PR description: include a short usage example, referenced test run, and the resource lines you used.

Examples to follow in PRs
- Add comments like: `// Parsing fmt chunk (see wav-resources/WAVE File Format.html, section "fmt ")`.
- Provide a tiny usage snippet at the top of new headers showing how to instantiate/parse a file.

If anything is missing in these instructions, or if you want me to scaffold a minimal header/test/build example, say which approach you prefer (CMake vs. single-file examples vs. header-only tests).
