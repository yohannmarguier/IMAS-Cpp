Any changes should be also refelcted in CLAUDE.md

## What this repository is

IMAS-Cpp is the C++ High Level Interface (HLI) of the IMAS Access Layer. Almost none of the
C++ library is written by hand: the library is **generated at build time** by XSLT
stylesheets that transform the IMAS Data Dictionary (`IDSDef.xml`) into C++ classes.
The handwritten part is tiny (`src/`, `tests/generator/helper.*`, `examples/`); the
stylesheets in the repository root *are* the source of truth for the API.

Sibling repositories it depends on: **IMAS-Core** (`al` target, low-level I/O and backends),
**IMAS-Data-Dictionary** (`IDSDef.xml`), **IMAS-Core-Plugins** (optional).

## Build

CMake ≥ 3.16, out-of-source only (in-source builds hard-error). Dependencies are fetched
via `FetchContent` by default.

```bash
# Fetch dependencies from GitHub (default; use --preset=https for HTTPS remotes)
cmake -B build -D CMAKE_INSTALL_PREFIX=$PWD/test-install
make -C build -j8 al-cpp al-identifiers-cpp
make -C build all           # examples + test suite
make -C build install
```

Key configure options (defaults in `common/cmake/ALCommonConfig.cmake`; documented in
`doc/doc_common/building_installing.rst` — keep both in sync when changing defaults):

| Option | Meaning |
| --- | --- |
| `AL_DOWNLOAD_DEPENDENCIES=ON` | Clone IMAS-Core / DD / plugins into `build/_deps` |
| `AL_DEVELOPMENT_LAYOUT=ON` | Use sibling checkouts `../IMAS-Core`, `../IMAS-Data-Dictionary`, `../IMAS-Core-Plugins` (forces `AL_DOWNLOAD_DEPENDENCIES=OFF`) |
| both `OFF` | Expect `al-core` and the DD as installed pkg-config / module packages |
| `AL_CORE_VERSION`, `DD_VERSION`, `AL_PLUGINS_VERSION` | Git ref per dependency (ignored in development layout) |
| `AL_TESTS`, `AL_EXAMPLES` | Build `cpp-TestSuite` / `examples` and register ctest tests |
| `AL_PLUGINS=OFF` | Also run every example a second time with plugins enabled |
| `AL_BACKEND_MDSPLUS`, `AL_BACKEND_HDF5`, `AL_BACKEND_UDA` | Passed through to IMAS-Core |
| `AL_HLI_DOCS`, `AL_DOCS_ONLY` | Sphinx docs (target `al-cpp-docs`) |

`ci/build_and_test.sh` is the reference full build (SDCC modules, all backends);
`ci/build_docs.sh` builds only the documentation.

**Memory/time**: `al-cpp` and `cpp-TestSuite` are compiled with `-O0` on purpose — the
generated translation units are so large that optimizing or adding debug symbols exhausts
compiler memory. Build `cpp-TestSuite` without `-j`.

## Tests

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build -R example-cpp-test_magnetics_put --output-on-failure   # single test
ctest --test-dir build -N                                                     # list tests
```

Two test groups:
- `cpp-TestSuite` — one huge generated executable (`tests/generator/TestSuite.xsl` +
  `helper.cpp`) that round-trips every field of every IDS. Disabled unless
  `AL_BACKEND_MDSPLUS=ON`.
- `example-cpp-<name>` — one test per `examples/*.cpp`.

Things that bite:
- Tests pass/fail on **output pattern matching**, not exit code: `FAIL_REGULAR_EXPRESSION`
  in `common/cmake/ALExampleUtilities.cmake` fails any test printing `error`, `fault`,
  `exception`, `failed`, `abort`, `dump`, … (case-insensitive). Do not add such words to
  example `printf` output.
- put/get pairs are wired with ctest **fixtures** (`test_x_put` sets up, `test_x_get`
  requires it), derived by stripping `put`/`get` from the test name. Tests share data
  entries, so they are not parallel-safe.
- Examples hardcode URIs like `imas:mdsplus?path=./test_db_...`; the MDSplus backend and
  its built models (`MDSPLUS_MODELS_PATH`, formerly `ids_path`) are required for most of
  them. CI points `USER` at a scratch `testdb/` directory before running the tests.
- Adding `examples/foo.cpp` requires adding `foo` to the `TESTS` list in
  `examples/CMakeLists.txt` — `error_on_missing_tests()` fails configuration otherwise.

## Code generation architecture

Everything under `build/` is generated; never edit generated files, edit the stylesheet.
XSLT is run through `common/xsltproc.py`, a Saxon-HE CLI replacement using the `saxonche`
Python wheel, installed into a venv at `build/dd_build_env` at configure time.

| Stylesheet | Produces | Consumed by |
| --- | --- | --- |
| `IDSDef2CPPClasses.xsl` | `build/src/ALClasses.h`, `build/src/ids/<ids>_IDSBase.h` | `al-cpp` |
| `IDSDef2CPPMethods.xsl` | `build/src/ALMethods.cpp`, `build/src/ids/<ids>_IDSBase.cpp` | `al-cpp` |
| `identifiers.xsl` (+ `common/identifiers.common.xsl`) | `build/identifiers/src/*_identifier.{h,cpp}` | `al-identifiers-cpp` |
| `tests/generator/TestSuite.xsl` | `build/tests/generator/TestSuite.cpp` | `cpp-TestSuite` |
| `common/list_idss.xsl`, `common/dd_version.xsl` | `IDS_NAMES`, `DD_VERSION` at configure time | CMake |

`IDSDef2CPPMethods.xsl` (~135 kB) is where the real logic lives, organised as XSLT
templates with `mode` names that mirror the generated methods: `CLASS_DEFINITION`,
`METHOD_PUT` / `PUT_SINGLE`, `METHOD_GET` / `GET_SINGLE`, `METHOD_PUT_SLICE`,
`METHOD_VALIDATE` and the `VALIDATE_*` family, `RESET`, `DELETE`, `DUMP`,
`DISCARD_CACHE`. When changing behaviour of a generated method, find the matching mode.
Both stylesheets take `DD_GIT_DESCRIBE` and `AL_GIT_DESCRIBE` parameters, which end up as
the `al_dd_version` / `al_cpp_version` constants.

Regeneration is driven by a dummy output file (`build/src/dummy.txt`) behind the
`al-cpp-sources` target, so generation reruns only when a stylesheet or `IDSDef.xml`
changes — but it then rebuilds the whole (slow) library. `touch`ing a stylesheet is the
way to force it.

## Generated API shape

All symbols live in namespace `IdsNs`. `IdsNs::IDS` is the root handle (open/close a data
entry) and exposes one member per IDS named `_<ids_name>`; each is a subclass of
`IdsNs::Ids` (`src/IdsDef.h`) with `get`/`put`/`getSlice`/`putSlice`/`partialGet`/
`validate`/`deleteAll`/`isDefined`. Fields are plain members; arrays are
`IMASArray<T,N>` (`src/ALDef.h`), a `blitz::Array` subclass that tracks its memory
deletion policy so data allocated by the low-level layer is freed correctly. Arrays of
structures are resized with `.resize(n)` and indexed with `(i)`, blitz-style.
`EMPTY_INT` / `EMPTY_FLOAT` / `EMPTY_DOUBLE` mark unset scalars.

## Shared assets — edit with care

`common/` and `doc/doc_common/` are shared with the other Access Layer HLI repositories
(same files are vendored there). `common/CMakeLists.txt` is explicitly written to be
consumed via `FetchContent` from other components, and generic text in `doc_common`
covers all HLIs. Changes there are cross-repository changes; prefer putting C++-specific
things in the repository root, `doc/*.rst`, or `common/cmake/ALLocalPaths.cmake`.

The project version comes from `git describe --tags` via
`common/cmake/ALDetermineVersion.cmake` (`.gitattributes` `export-subst` covers tarball
builds), and the installed library name embeds the DD version
(`libal-cpp-<DD_VERSION>`), with `imas-*.pc` → `al-*.pc` symlinks installed for backward
compatibility.

## Contributing workflow

Work on a feature branch off `develop`; pull requests target `develop` (`main` holds
releases). See `CONTRIBUTING.md` — an issue is expected before non-trivial changes.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).

## Agent skills

### Issue tracker

Issues live in GitHub Issues (`gh` CLI). See `docs/agents/issue-tracker.md`.

### Triage labels

Default label vocabulary (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout — `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.

