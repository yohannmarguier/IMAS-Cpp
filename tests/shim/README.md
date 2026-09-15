# Multiversion shim conformance suite

This directory holds the Tier-1 shim conformance suite described in
`docs/SHIM_SUITE_CONVENTION.md`. It registers only when
`AL_USE_MULTIVERSION_SHIM=ON`; with shim mode off the registered test list is
exactly what it was before this suite existed.

## Scope so far (issues #10, #11, #12)

Issue #10 registered the suite's scaffold and its first test.
Issue #11 added the shared comparison oracle every fixture-driven family
(issues #13, #14, #24) will read its verdicts from. Issue #12 vendors the
fixture pair those families read and the two isolation helpers a write and a
read scenario each need:

- `cpp-test-shim-linkage` (F1.1, `harness`): inspects the built `al-cpp`
  library's dynamic dependencies with `otool`/`objdump` and fails unless the
  multiversion shim's library name appears. Until this passes, nothing else
  this suite will ever say means anything -- every test in this repository's
  own suite passes just as well against a build that resolved its symbols
  straight to IMAS-Core and converted nothing.
- `cpp-test-shim-run-guard` (`harness`): a self-test of `shim_run_guard.h`,
  the shared assertion-count guard every later contract-assertion and
  behaviour-pin program in this suite is expected to use.
- `al_cpp_shim_private_loss_log()`, the CMake function later tickets use to
  give a test its own loss-log directory, cleaned before each run.
- `shim_comparator.h` (F1.2/F1.3): the one shared comparator every later
  contract-assertion program compares its oracle and converted readings
  through. `ShimTest::Verdict` is the closed, seven-member verdict set from
  `docs/SHIM_SUITE_CONVENTION.md` S4.1; `ShimTest::OracleReading` and
  `ShimTest::ConvertedReading` are distinct, non-interconvertible types, so a
  `Compare()` call with the two arguments swapped is a compile error, not a
  convention a reviewer has to notice broken.
- `cpp-test-shim-comparison` (F1.2, `harness`): the comparator's own
  synthetic truth table, driven from literals -- every verdict, both
  orientations of the asymmetric ones, the empty-vs-empty presence trap, and
  the shape-mismatch case. No rule table, no fixture, no pulse: a
  pulse-based test can never prove its own oracle right.
- `cpp-test-shim-verdict-orientation` (F1.3, `harness`): the source check
  covering the one mistake the type system above cannot catch -- a call that
  spells both `oracle=`/`converted=` comments correctly but wraps the wrong
  reading in each. It counts how many named sides it finds across this
  directory and fails below a floor, so it cannot pass by finding nothing;
  the floor rises as later tickets add their own `Compare()` call sites.
- **`../../imas-python-fixtures/`** (vendored verbatim from IMAS-Fortran, see
  its own README): two completely filled `equilibrium` HDF5 pulses, DD
  3.39.0 and DD 4.1.1, describing one equilibrium, generated from a single
  shared value table by two modules that decide only *where* each value
  goes. Every fixture-driven family this suite still needs (issues #13, #14,
  #24) reads its expected values out of these pulses rather than out of a
  literal (`docs/SHIM_SUITE_CONVENTION.md` D4).
- `cpp-test-shim-fixture-provenance` (F1.4, `harness`): regenerates the pair
  outside the checkout with `imas-python-fixtures/verify_fixtures.sh` and
  compares every HDF5 dataset against the checked-in oracle with `h5diff`.
  Registers only when the fixtures' own venv (`imas-python-fixtures/.venv`,
  see its README) and `h5diff` are both present; a missing prerequisite
  skips the test rather than passing it without comparing.
- `derive_stamp_variant.py` / `cpp-test-shim-stamp-variants`: three
  stamp-state fixtures -- stamp **absent** (the version dataset deleted),
  stamp **malformed** (a value that fails the version grammar), and stamp
  **mismatched with no artifact** (a grammar-valid known release neither
  this suite's HLI version nor DD 3.39.0) -- derived from the checked-in DD
  3.39.0 pulse at build time via a `cpp-test-shim-stamp-fixtures` `ALL`
  target, never committed. `cpp-test-shim-stamp-variants` (`harness`) checks
  the three keep the states F2.3 and F3.1 need distinct, rather than trusting
  the derivation script did. Registers only when the venv additionally has
  `h5py`.
- `al_cpp_shim_private_fixture_copy()` / `cpp-test-shim-fixture-copy`: the
  CMake function a write scenario (issue #24) uses to get a freshly made
  private copy of a fixture, so parallel scenarios cannot collide and a
  failed run leaves no poison. The self-test exercises it against a
  synthetic directory, not a pulse.
- `fixture_digest.cmake` / `verify_fixture_unchanged.cmake` /
  `cpp-test-shim-fixture-digest`: the content-digest helper a read scenario
  (issues #13, #14, #24) wraps its executable in to prove the checked-in
  fixture it read is unchanged afterwards -- content, not mtime, because an
  HDF5 rewrite can leave both alone. The self-test exercises it against a
  synthetic directory too.

None of `cpp-test-shim-linkage`, `cpp-test-shim-run-guard`, the comparator
tests, or the fixture/stamp-variant tests above call into the shim's runtime
(fixture provenance and the stamp-variant check inspect and derive HDF5
directly; the copy and digest self-tests use a synthetic directory), so this
ticket still declares no **profile** (Tier-1 read tolerance) or **direction**
(which DD pair, which way) -- see `docs/SHIM_SUITE_CONVENTION.md` S2.3 and
D2. Issue #24 owns that declaration, together with the red list and the
coverage boundaries, and this section should be replaced with it as that
work lands.

IMAS-Core is pinned to a fork commit carrying the path-aware HDF5 delete fix
(IMAS-Core #63/#64) whenever `AL_USE_MULTIVERSION_SHIM=ON` -- see the comment
at the top of the repository's `CMakeLists.txt`. Without it, `F6.4
full-put-stamp` (not yet implemented here) would be red for a reason in
neither this repository nor the shim.

## Running the labels

From a configured shim build directory:

```sh
ctest --test-dir <shim-build> -L shim --output-on-failure
ctest --test-dir <shim-build> -L harness --output-on-failure
```

`contract-assertion` and `behaviour-pin` tests do not exist here yet; when
they land, they follow `docs/SHIM_SUITE_CONVENTION.md` S1 D5: a
`contract-assertion` stays red while the shim disagrees and is never
inverted, quarantined, or softened to match observed behaviour.

This suite is not wired into CI. Wiring it in is a decision to take
explicitly, once there is a red list to publish (S1 D5) and a loaded
IMAS-Core to record alongside it.
