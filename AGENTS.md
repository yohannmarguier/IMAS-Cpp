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
| `AL_USE_MULTIVERSION_SHIM=OFF` | Link C++ calls to the shim found through `CMAKE_PREFIX_PATH` when enabled; Core is still acquired for headers and runtime use |
| `AL_TESTS`, `AL_EXAMPLES` | Build `cpp-TestSuite` / `examples` and register ctest tests |
| `AL_PLUGINS=OFF` | Also run every example a second time with plugins enabled |
| `AL_BACKEND_MDSPLUS`, `AL_BACKEND_HDF5`, `AL_BACKEND_UDA` | Passed through to IMAS-Core |
| `AL_HLI_DOCS`, `AL_DOCS_ONLY` | Sphinx docs (target `al-cpp-docs`) |

`ci/build_and_test.sh` is the reference full build (SDCC modules, all backends);
`ci/build_docs.sh` builds only the documentation.

Shim linkage is selected in `cmake/ALCppCoreLinkage.cmake`. The generator passes
`uri.c_str()` in `IDS::open(std::string)` to select the mirrored C ABI rather than
Core's C++ overload. No public API changes are needed. In shim mode only Core's
include directories propagate;
the C++ pkg-config dependency becomes `imas-mvdd-loader` to avoid linking Core
directly in consumers. A source-built Core remains a build dependency of
`al-cpp` and installs its headers alongside the HLI. An installed Core contributes
only compiler flags to the C++ pkg-config file. At runtime set
`IMAS_CORE_LIBRARY` to the real Core shared library when it is not on the loader
search path, and `IMAS_MVDD_HLI_DD_VERSION` to the HLI's DD version to enable
conversion. `AL_CPP_SHIM_TEST_ENVIRONMENT`, set in the same file, injects both
into the ctest environment of `examples/` and `tests/generator/`, because a
source-built Core sits in the build tree and is not on the loader search path —
without it every mirrored call fails (`getALVersion()` returns `NULL`).
No new shim functions are wrapped. The documented `build-shim/`
directory is ignored by Git. See `doc/building_installing.rst`.

**Memory/time**: `al-cpp` and `cpp-TestSuite` are compiled with `-O0` on purpose — the
generated translation units are so large that optimizing or adding debug symbols exhausts
compiler memory. Build `cpp-TestSuite` without `-j`.

## Tests

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build -R example-cpp-test_magnetics_put --output-on-failure   # single test
ctest --test-dir build -N                                                     # list tests
```

Three test groups:
- `cpp-TestSuite` — one huge generated executable (`tests/generator/TestSuite.xsl` +
  `helper.cpp`) that round-trips every field of every IDS. Disabled unless
  `AL_BACKEND_MDSPLUS=ON`.
- `example-cpp-<name>` — one test per `examples/*.cpp`.
- `cpp-test-shim-*` (`tests/shim/`) — the Tier-1 multiversion-shim conformance
  suite from `docs/SHIM_SUITE_CONVENTION.md`. Registers only when
  `AL_USE_MULTIVERSION_SHIM=ON`; not wired into CI. Labelled `shim` plus exactly
  one of `contract-assertion`, `behaviour-pin`, `harness`. Refusals and other
  error-shaped output are legitimate here, so `common/cmake/
  ALExampleUtilities.cmake`'s `FAIL_REGULAR_EXPRESSION` (which fails any test
  printing "error") is never used for it — a program's own exit status is the
  pass condition, paired where needed with a distinctive `*-FAILURE` marker
  in its own `FAIL_REGULAR_EXPRESSION`. See `tests/shim/README.md`. Its
  fixture-driven tests read `imas-python-fixtures/` (vendored from
  IMAS-Fortran, not generated here — see that directory's README), whose
  provenance and derived stamp-state variants are themselves registered
  tests, gated on a Python venv with `imas-python`/`h5py` and `h5diff` being
  present.
  `cpp-test-shim-roundtrip-cross-dd` and `cpp-test-shim-roundtrip-same-dd`
  are paired registrations of one slice-append program (issue #21): each has
  a fresh private fixture and loss-log directory; the DD 3.39.0 run explicitly
  permits `PARTIAL_READ`, while the DD 4.1.1 control requires clean success.
  Both assert exact time-slice/time-base growth, unchanged time mode, and a
  curated COCOS-mapped `psi_axis` round trip. The test is a consistency check,
  not evidence of the native on-disk stored path or sign.
  `cpp-test-shim-refusal-channels` (issue #18; HDF5 builds) asserts the map's
  one `retyped` rule -- `grids_ggd/grid/space/coordinates_type` -- is reported
  on all three refusal channels, not merely tolerated: the value is left
  absent, the path is named in the skipped-path record, and the read reports
  `PARTIAL_READ`. It also asserts the refusal was absorbed at the field rather
  than by truncating its surroundings (the enclosing containers survived, a
  field read after it in the same structure arrived, and a field served later
  in the traversal still agrees with the oracle). The two reads use
  independent `IdsNs::IDS` objects, so the oracle read cannot reach the
  converted read's own record; the test still copies that record out first,
  matching the ordering the shim suite convention states (the record resets
  at the start of each root operation), so the assertion stays correct if a
  future revision shares one object across both reads. `tests/shim/
  shim_refusal_match.h` is the shared record-matching predicate: it matches
  the operation, the full DD path (on the tail of the message's `DD path: `
  field, which survives truncation and tolerates a future generator
  prefixing it), the reason substring, and the refusal-status band. F4.4 as
  documented also covers the four unit-`redefined` globs, served with no
  refusal record; that half is not implemented here (issue #19).

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
The named `HANDLE_AOS_OPEN_STATUS` template centralises the shared read/write refusal
decision emitted after every array-of-structures open; keep branch-specific traversal in
`GET_SINGLE` and `PUT_SINGLE` rather than duplicating that policy block.
Both stylesheets take `DD_GIT_DESCRIBE` and `AL_GIT_DESCRIBE` parameters, which end up as
the `al_dd_version` / `al_cpp_version` constants.

Read traversal refusal policy is generated in `GET_SINGLE`: only leaf `readData` calls and
the failure arm of `al_begin_arraystruct_action` call `Ids::mustAbort`. Those sites thread
the root IDS object's skipped-path record through every nested `get` call and return
`PARTIAL_READ` after a tolerated refusal. Time-mode reads, occurrence opens, iteration and
end-action calls, and readback-plugin bind/unbind remain fatal `isError` sites. Root `get`,
`getSample`, and `getSlice` clear the record before starting their traversal. `partialGet`
also clears it on entry, before plugin setup can fail, and its delegated `get` clears it
again before traversal. The direct refusal-policy test uses a generated concrete IDS base
as its narrow protected-member adapter and covers the refusal-band boundaries, an interior
value, operation tags, and preservation of the shim message.

Write/delete refusal policy is generated in `PUT_SINGLE` and `DELETE`: only leaf `writeData` /
`al_delete_data` calls and the failure arm of `al_begin_arraystruct_action` call
`Ids::mustAbort`. Nested `put`, `putSlice`, and `deleteAll` methods carry the root record;
tolerated paths return `PARTIAL_PUT`, with a `Write` or `Delete` tag. Root `put`, `putSlice`,
and `deleteAll` clear the record first; full `put` retains tolerated deletes that occur before
its writes. Occurrence opens, data-entry seams, iteration, and end-action calls remain fatal.
Writes are best effort: a refused `putSlice` has no rollback, so prior writes and the resized
array-of-structures remain on disk. Ordinary builds exercise the shared tolerance decision
through `cpp-test-refusal-policy`; `cpp-test-generated-write-refusal-policy` temporarily
checks generated write/delete wiring until the multiversion-shim conformance suite provides
equivalent executable coverage with a shim and mismatched pulse.

The public contract is documented in `doc/api_ids.rst`: the three-way status of `get`,
`getSlice`, `getSample`, `put`, `putSlice`, and `partialGet` (`0` success; `>0` completed
with refused paths; `<0` failure), `getSkippedPaths`/`getSkippedPathCount`, the `SkippedPath`
record (operation Read/Write/Delete, path relative to the enclosing context, message carrying
the full DD path, code), the record reset at the start of each root operation, and that
refused writes are best effort and not rolled back. `doc/api_constants.rst` documents
`PARTIAL_READ`/`PARTIAL_PUT`. In `tests/generator/helper.cpp`, `checkStatus` treats any
non-zero status (`status != 0`) as a failure for the whole suite, without printing anything.

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
