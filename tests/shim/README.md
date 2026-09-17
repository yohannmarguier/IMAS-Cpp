# Multiversion shim conformance suite

This directory holds the Tier-1 shim conformance suite described in
`docs/SHIM_SUITE_CONVENTION.md`. It registers only when
`AL_USE_MULTIVERSION_SHIM=ON`; with shim mode off the registered test list is
exactly what it was before this suite existed.

## Scope so far (issues #10, #11, #12, #13, #14, #15, #16, #17, #18, #19, #20, #21, #22, #23)

Issue #10 registered the suite's scaffold and its first test.
Issue #11 added the shared comparison oracle every fixture-driven family
(issues #13, #14, #21) will read its verdicts from. Issue #12 vendors the
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
  #21) reads its expected values out of these pulses rather than out of a
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
  CMake function a write scenario (issue #21) uses to get a freshly made
  private copy of a fixture, so parallel scenarios cannot collide and a
  failed run leaves no poison. The self-test exercises it against a
  synthetic directory, not a pulse.
- `fixture_digest.cmake` / `verify_fixture_unchanged.cmake` /
  `cpp-test-shim-fixture-digest`: the content-digest helper a read scenario
  (issues #13, #14, #24) wraps its executable in to prove the checked-in
  fixture it read is unchanged afterwards -- content, not mtime, because an
  HDF5 rewrite can leave both alone. The self-test exercises it against a
  synthetic directory too.
- `cpp-test-shim-stamp-malformed` (F3.1, `contract-assertion`): opens the
  derived malformed-stamp fixture's data entry successfully, then asserts the
  first occurrence-opening HLI call (`equilibrium.get`) returns a refusal-band
  status, leaves the IDS declared-but-empty and records no skipped path. Its
  frozen reason is the suite's one named exception to public-return-value-only
  assertions: `get` prints the occurrence-open message instead of returning
  it, so the fixture wrapper reads the program's standard output only after
  the executable has validated the refusal-band status and exited cleanly. It
  also gets a fresh private loss-log directory because its refused occurrence
  open may otherwise leave a loss log in CTest's shared working directory.
- `cpp-test-shim-version-unset`, `cpp-test-shim-stamp-equal`,
  `cpp-test-shim-stamp-absent`, and
  `cpp-test-shim-stamp-mismatch-no-artifact` (F2.1--F2.4,
  `contract-assertion`): four separately launched HDF5 reads covering every
  state in which the shim must forward untouched. Each proves clean success,
  no skipped paths, fixture data, an unchanged fixture, and an empty private
  loss-log directory. The version-unset registration composes an environment
  without `IMAS_MVDD_HLI_DD_VERSION`; it does not clear an injected value. The
  mismatch-without-artifact scenario also checks that the newer-DD-only
  `beta_tor_norm` remains absent, and its source header records why its
  otherwise indistinguishable result must remain a separate scenario.

- `cpp-test-shim-structural-rules` (F4.1, `contract-assertion`, HDF5 builds): reads the
  DD 3.39.0 fixture through the shim and the DD 4.1.1 fixture same-version,
  then checks all 23 structural rules at the public C++ HLI. Its hand-authored
  table has 23 structural, 30 COCOS, 13 right-only, and 5 refusal entries;
  every entry cites the conversion map or fixture provenance. The transcription
  audit records the externally reachable map's unresolved includes (including
  common renames), its actual 30 COCOS flips, and the two fixture-only
  negations inside DD-4-only structures. It also records that four historical
  chi-squared unit redefinitions now fall through as identical in the map.
  The map wins over the stale quoted count of 32. Each rule derives its expected verdict from the shared
  kind mapping, and a multi-leaf rule reports its first non-agreeing verdict.
- `cpp-test-shim-cocos-rules` (F4.2, `contract-assertion`, HDF5 builds): reads
  the same fixture pair through the public HLI and checks every one of the
  table's 30 map-declared COCOS paths. Each expects the shared kind mapping's
  `same` verdict. A stopped sign flip therefore reports the comparator's
  `no-flip` verdict as a failed assertion, naming the rule id, kind and cited
  map/fixture source. The program counts checked entries against the table's
  own size, so it cannot report coverage for the two fixture-only negations.
- `cpp-test-shim-right-only-rules` (F4.3, `contract-assertion`, HDF5 builds): reads the
  same two fixtures as F4.1 and checks all 13 `right_only` rules in the shared
  table. Expects `ONLY_ORACLE` per rule -- the shim correctly serves nothing,
  and the oracle holds a real value proving the comparison is not vacuous.
  Carries the D6 vacuity demonstration this family requires: one right-only
  rule's converted reading, already established served-nothing by the
  ordinary check above it, is put through the same `Compare()` predicate
  against a real oracle value taken from an unrelated structural-table entry
  (`identical-vacuum-r0`), and must disagree with that entry's own agreement
  expectation. Without that demonstration, a shim serving nothing at all
  would satisfy every right-only rule for the wrong reason. The one
  right-only path this test indexes through an array-of-structures element
  (`constraints/j_parallel`) guards the converted side's element access on
  its own extent instead of assuming it was resized, since the path has no
  DD 3 source to resize it from. Per docs/SHIM_SUITE_CONVENTION.md S2.1, a
  converted reading that turns out neither absent nor `ONLY_ORACLE` is
  printed as its own named finding, separate from the ordinary rule-mismatch
  message: a field reading back as a plausible-looking value instead of the
  invalid sentinel is a worse outcome than a wrong one and this suite's job
  is to say so, not fold it into an unremarkable failure. IMAS-Fortran found
  exactly this on its own HLI (five `right_only` paths reading back as
  uninitialised memory, one as four ASCII spaces landing on an integer
  field). On this C++ HLI, on `MacBook-Pro-de-Yohann.local` (Darwin 25.6.0,
  arm64) on 2026-09-16, every one of the 13 `right_only` paths read back as
  the DD invalid sentinel (`EMPTY_DOUBLE`/`EMPTY_INT`) on the converted side
  -- no finding was printed. Re-run the test before treating that as a
  statement about another build or host.
- `cpp-test-shim-refusal-channels` (F4.4, `contract-assertion`, HDF5 builds): the same two
  reads as F4.1, asserting that the map's `retyped` rule --
  `grids_ggd/grid/space/coordinates_type` (an int array in DD 3.39.0, an array
  of identifier structures in DD 4.1.1, no transformation reshapes one into
  the other) -- is reported on all three refusal channels rather than merely
  tolerated: the value is left absent, the path is named in the skipped-path
  record, and the read reports `PARTIAL_READ`. Three structural checks show
  the refusal was absorbed at the field, not by truncating its surroundings:
  the enclosing containers survived, a field read after it within the same
  structure arrived, and a field served later in the traversal (equilibrium's
  root `time`) still agrees with the oracle. The two reads use independent
  `IdsNs::IDS` objects (as F4.1 does), so the oracle read cannot reach the
  converted read's own record; the program still copies that record out
  before the oracle read runs, matching the ordering
  `docs/SHIM_SUITE_CONVENTION.md` S5.4 states -- the record resets at the
  start of each root operation -- so the assertion stays correct if a future
  revision shares one object across both reads. `tests/shim/shim_refusal_match.h` is the shared
  record-matching predicate the next refusal-family ticket reuses: it matches
  the operation, the full DD path (on the tail of the message's `DD path: `
  field, so it tolerates truncation and a future generator prefixing that
  field with more context), the reason substring, and the refusal-status
  band. It also checks all four unit-`redefined` globs: each value agrees with
  the oracle and has no read skipped-path record. The test rejects any record
  for the full DD path, which is stricter than recognizing only a particular
  reason or refusal-band code, while still reusing the shared full
  path/reason/band matcher; each rule-level failure names its id, kind, and
  citation, and the two assertions per rule are counted from the table size.
- `cpp-test-shim-nested-loss` (F5.1, `contract-assertion`, HDF5 builds,
  issue #20): the program reads the full older-DD pulse and checks only
  `PARTIAL_READ` and a nonempty skipped-path record. Its CMake wrapper requires
  a clean program exit and an unchanged fixture, then inspects exactly one
  file in a private directory cleaned before each run. It pins the format-1
  marker and line-five header, checks seven columns on every row, and compares
  the set of operation/fidelity/path triples for equality (duplicates allowed).
  The 14 `LOSSY` and three `UNMAPPABLE` rows are explained in
  `check_nested_loss_log.cmake`: the latter reflect array-of-structures open
  refusals, including the retyped coordinates container. This pins a refusal
  decision; a change to return empty containers may turn it red on an
  improvement. Suspect that decision before a mapping regression, and check
  DD shape before interpreting sibling fidelities. Historical unit-redefinition
  refusals are removed into a separate known-defect set and each is reported
  as `LOSS-LOG-REDEFINITION-FAILURE`, consistent with issue #19's served-value
  assertions. `cpp-test-shim-loss-log-harness` (`harness`) tests the wrapper
  from a literal report, including malformed files, exact-set differences,
  known defects, fixture mutation, and nonzero program exits.
- `cpp-test-shim-roundtrip-cross-dd` and `cpp-test-shim-roundtrip-same-dd`
  (F6.1--F6.2, `contract-assertion`): two registrations of one program that
  appends a curated `psi_axis` value at a COCOS sign-flip path, then reads the
  entire occurrence back. Each setup creates a fresh private fixture copy and
  loss-log directory. The older-DD run explicitly permits a partial read; the
  same-DD control explicitly requires a clean read. Both require the slice and
  time base to grow by one, preserve time mode, and return the appended time
  and value. The source header records why this consistency check cannot prove
  the stored path, raw sign, stamp, or candidate selection. A paired CTest
  fixture owns exactly the two program runs and records both controls before
  either named case can report a round-trip result without its control.

The round-trip pair establishes this suite's **Profile A** direction: a DD
4.1.1 HLI reads and writes DD 3.39.0 pulses, tolerating per-field refusals as
partial operations. Its same-DD DD 4.1.1 control proves that a successful
round trip did not merely avoid conversion. It does not establish the broader
read-rule catalogue or native on-disk assertions; those remain separate
families under `docs/SHIM_SUITE_CONVENTION.md` S5.

- `cpp-test-shim-torn-write` (F6.3, **`behaviour-pin`**): against a fresh
  private copy of the older-DD pulse, appends one slice carrying both the
  mapped, COCOS-flipped `global_quantities/psi_axis` and
  `global_quantities/rho_tor_boundary` -- a field the shim's own conversion
  map (`IMAS-Multiversion-DD-Loader`'s `docs/3.39.0--4.1.1.xml`, rule
  `new-global-quantities-rho-tor-boundary`) records as `right_only`: "path
  added in DD 3.40.0; no counterpart in 3.39.0", forward direction
  `unmappable`. Asserts the `putSlice` reports `PARTIAL_PUT`; that
  `getSkippedPaths()` names the specific refused `Write` of
  `global_quantities/rho_tor_boundary` (a slice put writes every field of the
  whole appended element, so other unmapped fields under it may also be
  refused -- this pin cares about one field by name, not about being the
  only refusal); and, on the read-back, that the time-slice and time-base
  containers each grew by exactly one, the mapped `psi_axis` value the
  refusal ran alongside is readable, and the refused field itself stayed
  empty. `verify_torn_write.cmake` also asserts the traversal's own
  `REFUSED WRITE: global_quantities/rho_tor_boundary` diagnostic
  (`IdsNs::Ids::mustAbort`, `src/IdsDef.cpp`) reached the program's standard
  output and not its standard error. This is a pin on an accepted
  limitation, not a statement that a torn write is desirable: the generated
  write traversal has no rollback, so a refusal partway through leaves
  everything already written on disk, and the container one element longer
  regardless, because the array-of-structures open widens it before any leaf
  write runs.

- `cpp-test-shim-full-put-stamp` (F6.4, `contract-assertion`): against a
  fresh private copy of the older-DD pulse, reads the occurrence, sets a
  marker value at `vacuum_toroidal_field/r0` -- the field the generated
  traversal reaches immediately after `ids_properties` -- and issues a
  **full put** (`put()`, not `putSlice()`; a slice put has an empty body for
  `ids_properties/version_put/data_dictionary`, the DD-version stamp, and
  never reaches it at all). A full put's `put()` opens by calling its own
  `deleteAll()`, which deletes every other field of the previous occurrence
  but refuses the delete that would remove the stamp while data remains;
  `put()` then rewrites every field, including the stamp itself hardcoded to
  this HLI's own compiled DD version, whose write is refused too under the
  mismatch. Asserts the refused delete and the refused write each on their
  own counter -- either refusal alone already makes the summary status
  partial, so the status cannot say whether both seams, or only one,
  refused -- then asserts the status as the derived `PARTIAL_PUT` outcome
  it is, and, on the read-back, that the marker field reads back (proving
  the traversal finished) and the stamp still names the fixture's own
  `3.39.0`, never this HLI's `4.1.1`. `verify_full_put_stamp.cmake`
  additionally asserts both `REFUSED DELETE:
  ids_properties/version_put/data_dictionary` and `REFUSED WRITE:
  ids_properties/version_put/data_dictionary` (`IdsNs::Ids::mustAbort`,
  `src/IdsDef.cpp`) reached the program's standard output and not its
  standard error. Unlike F6.3, this pins a **requirement** of the shim, not
  an accepted limitation.

IMAS-Core is pinned to a fork commit carrying the path-aware HDF5 delete fix
(IMAS-Core #63/#64) whenever `AL_USE_MULTIVERSION_SHIM=ON` -- see the comment
at the top of the repository's `CMakeLists.txt`. Without it, `F6.4
full-put-stamp` would be red for a reason in neither this repository nor the
shim: the first delete a full put issues would destroy the whole occurrence
instead of sparing the stamp, the refused stamp delete would then protect
nothing, the stamp probe that follows would find no occurrence, an absent
stamp is presumed to match, no conversion is armed, and every write becomes
an untranslated forward.

## Running the labels

From a configured shim build directory:

```sh
ctest --test-dir <shim-build> -L shim --output-on-failure
ctest --test-dir <shim-build> -L harness --output-on-failure
```

The five F2/F3 `contract-assertion` tests follow
`docs/SHIM_SUITE_CONVENTION.md` S1 D5 and stay red while the shim disagrees,
never inverted, quarantined, or softened to match observed behaviour.
`cpp-test-shim-torn-write` (F6.3) is this suite's one `behaviour-pin`: it
preserves an accepted limitation (no rollback on a refused write) rather than
asserting a requirement of the shim, so nothing here should ever change to
give it atomic rollback.

## Contract assertions known to be red

There are none. `cpp-test-shim-stamp-malformed` passed on 2026-09-15 on
`MacBook-Pro-de-Yohann.local` (Darwin 25.6.0, arm64), after the fixture
provenance and stamp-variant checks passed. The shim loaded IMAS-Core commit
`dae4abdd9428bd28f47063f8f575bdc8abd915f2` from
`cmake-build-debug-shim/_deps/al-core-build/libal.5.7.2.86.dylib`. Re-run the
shim-labelled suite and its linkage check before treating that observation as
a statement about another shim or IMAS-Core build.

This suite is not wired into CI. Wiring it in is a decision to take
explicitly, once its profile, direction, and coverage boundaries are
published (S1 D5) alongside the loaded IMAS-Core.
