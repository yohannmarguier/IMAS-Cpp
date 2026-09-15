# Multiversion shim conformance suite

This directory holds the Tier-1 shim conformance suite described in
`docs/SHIM_SUITE_CONVENTION.md`. It registers only when
`AL_USE_MULTIVERSION_SHIM=ON`; with shim mode off the registered test list is
exactly what it was before this suite existed.

## Scope so far (issues #10, #11)

Issue #10 registered the suite's scaffold and its first test.
Issue #11 adds the shared comparison oracle every fixture-driven family
(issues #12-14, #24) will read its verdicts from:

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

Neither `cpp-test-shim-linkage` nor `cpp-test-shim-run-guard` nor the
comparator tests call into the shim's runtime, so this ticket still declares
no **profile** (Tier-1 read tolerance) or **direction** (which DD pair, which
way) -- see `docs/SHIM_SUITE_CONVENTION.md` S2.3 and D2. The tickets that add
the fixture-driven families (issues #12-14, #24) own those declarations, and
this section should be replaced with them as that work lands.

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
