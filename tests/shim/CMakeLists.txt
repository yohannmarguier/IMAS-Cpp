# The shim conformance suite scaffold (issue #10), plus the harness-integrity
# comparator (issue #11).
#
# Registration order below follows docs/SHIM_SUITE_CONVENTION.md S5.1's
# Family 1 (harness integrity): linkage, then the comparator's own truth
# table, then the source check that covers what the comparator's type system
# cannot. Until linkage passes, nothing else the suite will ever say means
# anything: every functional test in this repository's own suite passes just
# as well against a build that resolved its symbols straight to IMAS-Core and
# converted nothing. See tests/shim/README.md for what is and is not covered
# so far.
#
# The whole file is registered only in a shim build: with
# AL_USE_MULTIVERSION_SHIM off there is nothing to assert and the test list
# stays exactly what it was.

if( WIN32 )
  # The shim resolves AL core through dlopen/dlsym, so a shim build is
  # POSIX-only.
  message( STATUS "Not registering the shim test suite: not supported on Windows" )
  return()
endif()

# otool and objdump report a shared library's dependencies in different
# formats, but both spell the dependency's file name, which is all the
# linkage test looks for.
if( APPLE )
  find_program( AL_SHIM_INSPECT_TOOL otool )
  set( AL_SHIM_INSPECT_ARGS -L )
elseif( CMAKE_OBJDUMP )
  # What CMake itself uses for dependency inspection.
  set( AL_SHIM_INSPECT_TOOL ${CMAKE_OBJDUMP} )
  set( AL_SHIM_INSPECT_ARGS -p )
else()
  find_program( AL_SHIM_INSPECT_TOOL objdump )
  set( AL_SHIM_INSPECT_ARGS -p )
endif()

if( NOT AL_SHIM_INSPECT_TOOL )
  # This return() skips the whole suite, not only the linkage test, because
  # every test below is registered after it. A missing prerequisite must
  # skip registration, never register a test that would pass without
  # checking anything.
  message( STATUS "Not registering the shim test suite: no dependency inspection tool found" )
  return()
endif()

# F1.1 linkage (docs/SHIM_SUITE_CONVENTION.md S5.1): inspect the built al-cpp
# library's dynamic dependencies and require the shim's library name to
# appear. Functional tests pass against a build that never loaded the shim;
# a dependency listing is the only honest check.
add_test( NAME cpp-test-shim-linkage
  COMMAND ${CMAKE_COMMAND}
    -D "LIBRARY=$<TARGET_FILE:al-cpp>"
    -D "INSPECT_TOOL=${AL_SHIM_INSPECT_TOOL}"
    -D "INSPECT_ARGS=${AL_SHIM_INSPECT_ARGS}"
    -D "SHIM_LIBRARY_NAME=${AL_SHIM_LIBRARY_NAME}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_shim_linkage.cmake"
)
set_tests_properties( cpp-test-shim-linkage PROPERTIES
  LABELS "shim;harness"
)

# The shared run guard every later contract-assertion / behaviour-pin
# program in this suite is expected to use (docs/SHIM_SUITE_CONVENTION.md
# D6, shim_run_guard.h): it counts the assertions a program actually ran and
# fails if that differs from a number the program states up front, so a
# check lost in a later edit fails the run instead of quietly shrinking it.
# Tested here in isolation, from literals -- a pulse-based test can never
# prove its own guard right, the same reasoning S4.4 applies to the
# comparison oracle.
add_executable( cpp-test-shim-run-guard test_shim_run_guard.cpp )
add_test( NAME cpp-test-shim-run-guard COMMAND cpp-test-shim-run-guard )
set_tests_properties( cpp-test-shim-run-guard PROPERTIES
  LABELS "shim;harness"
  FAIL_REGULAR_EXPRESSION "RUN-GUARD-FAILURE"
)

# F1.2 comparison (docs/SHIM_SUITE_CONVENTION.md S4.4, S5.1): the shared
# comparator's own synthetic truth table, driven from literals -- no rule
# table, no fixture, no pulse, because a pulse-based test can never prove
# its own oracle right.
add_executable( cpp-test-shim-comparator test_shim_comparator.cpp )
add_test( NAME cpp-test-shim-comparison COMMAND cpp-test-shim-comparator )
set_tests_properties( cpp-test-shim-comparison PROPERTIES
  LABELS "shim;harness"
  FAIL_REGULAR_EXPRESSION "COMPARATOR-FAILURE"
)

# F1.3 verdict-orientation (docs/SHIM_SUITE_CONVENTION.md S4.3, S5.1): the
# residual hole the comparator's distinct wrapper types cannot close on
# their own -- a call that spells both `oracle=`/`converted=` comments but
# wraps the wrong reading in each. Counts the named sides this ticket's own
# comparator test contributes; later tickets' rule programs raise the floor
# as they add Compare() call sites of their own.
# Eleven sides are contributed by the comparator self-test; F4.1 adds the
# fixture-driven comparison site's twelfth named pair.
set( AL_SHIM_VERDICT_ORIENTATION_FLOOR 12 )
add_test( NAME cpp-test-shim-verdict-orientation
  COMMAND ${CMAKE_COMMAND}
    -D "SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    -D "FLOOR=${AL_SHIM_VERDICT_ORIENTATION_FLOOR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_verdict_orientation.cmake"
)
set_tests_properties( cpp-test-shim-verdict-orientation PROPERTIES
  LABELS "shim;harness"
)

# F1.4 fixture-provenance (docs/SHIM_SUITE_CONVENTION.md S3.2, S5.1): the
# checked-in equilibrium pulses must be what imas-python-fixtures/ produces,
# or every rule-table assertion this suite will ever make (issues #13--15,
# #21, #24) is against numbers nobody can re-derive. verify_fixtures.sh
# regenerates them outside the checkout and compares every HDF5 dataset with
# h5diff -- registering it here is what turns that from a script somebody
# may remember to run into a criterion the suite enforces.
#
# It needs the fixtures' own virtualenv (imas-python-fixtures/README.md) and
# h5diff, neither of which this project builds, so it registers only when
# both are present -- the same pattern as the linkage test above. A missing
# venv means the check is skipped, never that it silently passes.
set( AL_SHIM_FIXTURE_VENV_PYTHON "${CMAKE_SOURCE_DIR}/imas-python-fixtures/.venv/bin/python" )
find_program( AL_SHIM_H5DIFF h5diff )
if( EXISTS "${AL_SHIM_FIXTURE_VENV_PYTHON}" AND AL_SHIM_H5DIFF )
  add_test( NAME cpp-test-shim-fixture-provenance
    COMMAND "${CMAKE_SOURCE_DIR}/imas-python-fixtures/verify_fixtures.sh"
  )
  set_tests_properties( cpp-test-shim-fixture-provenance PROPERTIES
    LABELS "shim;harness"
  )
else()
  message( STATUS
    "Not registering cpp-test-shim-fixture-provenance: needs "
    "imas-python-fixtures/.venv (see its README) and h5diff" )
endif()

# The root every fixture-driven family in this suite (issues #13--15, #21,
# #24)
# reads from.
set( AL_SHIM_FIXTURE_ROOT "${CMAKE_SOURCE_DIR}/imas-python-fixtures/fixtures" )

# Two harness self-tests for helpers this ticket adds and later tickets rely
# on: a content-digest check that a read scenario's fixture is unchanged
# afterwards (docs/SHIM_SUITE_CONVENTION.md S3.3), and a private, freshly
# made copy for a write scenario (S3.3, S6). Both are exercised here against
# a synthetic directory, not a pulse -- a pulse-based test can never prove
# its own harness right, the same reasoning S4.4 gives the comparator.
add_test( NAME cpp-test-shim-fixture-digest
  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/check_fixture_digest.cmake"
)
set_tests_properties( cpp-test-shim-fixture-digest PROPERTIES
  LABELS "shim;harness"
  FAIL_REGULAR_EXPRESSION "FIXTURE-DIGEST-FAILURE"
)

add_test( NAME cpp-test-shim-fixture-copy
  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/check_fixture_copy.cmake"
)
set_tests_properties( cpp-test-shim-fixture-copy PROPERTIES
  LABELS "shim;harness"
  FAIL_REGULAR_EXPRESSION "FIXTURE-COPY-FAILURE"
)

# Give one test a private, empty loss-log directory.
#
# Leaving IMAS_MVDD_LOSS_LOG_DIR unset is not neutral: docs/
# SHIM_INTEGRATION_CONTRACT.md section 7 says the shim then writes into the
# current working directory, which for a ctest run is the build tree, where
# the file is left for whichever test globs there next. Every test whose
# read can be non-exact therefore needs a directory of its own -- see
# docs/SHIM_SUITE_CONVENTION.md S3.4 -- whether or not it asserts on it.
#
# None of the tests registered above call into the shim's runtime (linkage
# only inspects the built library; run-guard and the comparator tests make
# no AL call), so nothing here calls this function yet. It is part of this
# ticket's scaffold anyway: the convention sets this rule "once, relied on
# by every later ticket", and the contract-assertion and behaviour-pin
# families (issues #12--15, #21, #24) need it from their first test.
function( al_cpp_shim_private_loss_log test_name out_var )
  set( _dir "${CMAKE_CURRENT_BINARY_DIR}/${test_name}-loss-log" )
  add_test( NAME ${test_name}-loss-log-clean
    COMMAND ${CMAKE_COMMAND}
      -D "LOSS_LOG_DIR=${_dir}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/clean_loss_log_dir.cmake"
  )
  # No second label (contract-assertion/behaviour-pin/harness): this is
  # FIXTURES_SETUP plumbing that runs ahead of the test it serves, not
  # itself a statement about the shim, so `-L <second-label>` selecting only
  # statements about the agreement should not need to see it. IMAS-Fortran's
  # reference suite draws the same line.
  set_tests_properties( ${test_name}-loss-log-clean PROPERTIES
    FIXTURES_SETUP ${test_name}-loss-log
    LABELS "shim"
  )
  set( ${out_var} "${_dir}" PARENT_SCOPE )
endfunction()

# A write scenario's private fixture copy (docs/SHIM_SUITE_CONVENTION.md
# S3.3): "A write test MUST run against a private copy made fresh before
# each run, so scenarios can run in parallel and a failed run leaves no
# poison." The round-trip pair below uses it; torn-write and full-put-stamp
# remain issue #24's. The helper ships with this ticket for the same reason
# al_cpp_shim_private_loss_log did with #10: the family that needs it should
# need nothing else from the scaffold.
function( al_cpp_shim_private_fixture_copy test_name source out_var )
  set( _dir "${CMAKE_CURRENT_BINARY_DIR}/${test_name}-fixture" )
  add_test( NAME ${test_name}-fixture-copy
    COMMAND ${CMAKE_COMMAND}
      -D "SOURCE=${source}"
      -D "DESTINATION=${_dir}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_fixture.cmake"
  )
  # No second label, for the same reason as the loss-log setup test above:
  # this is FIXTURES_SETUP plumbing, not itself a statement about the shim.
  set_tests_properties( ${test_name}-fixture-copy PROPERTIES
    FIXTURES_SETUP ${test_name}-fixture
    LABELS "shim"
  )
  set( ${out_var} "${_dir}" PARENT_SCOPE )
endfunction()

# F3.1's program has no fixture-generation dependency at compile time. Keep
# the target available to typecheck in every shim build; registration below is
# still gated on the tooling that derives its malformed-stamp input.
add_executable( cpp-test-shim-stamp-malformed test_shim_stamp_malformed.cpp )
target_link_libraries( cpp-test-shim-stamp-malformed PRIVATE al-cpp )

# Stamp-state variants (docs/SHIM_SUITE_CONVENTION.md S3.3): three fixture
# states the passthrough and occurrence-open-refusal families (issues #13,
# #14) read while opening an occurrence, so they must exist before those
# tests' programs begin. Derived at build time from the checked-in DD
# 3.39.0 pulse -- absent (the stamp dataset deleted), malformed (a value
# that fails the version grammar), and mismatched with no artifact (a
# grammar-valid known release that is neither this suite's HLI version nor
# DD 3.39.0) -- and never committed: a ~40-line script copying the pulse and
# editing one dataset is enough; three more HDF5 pulses in the repository is
# not.
#
# Gated on the same fixtures venv as F1.4 above, plus h5py specifically:
# regenerating the pair only needs imas-python, but editing one dataset in a
# copy needs h5py, and the two are not the same dependency set.
set( AL_SHIM_STAMP_VARIANT_TOOLING_AVAILABLE FALSE )
if( EXISTS "${AL_SHIM_FIXTURE_VENV_PYTHON}" )
  execute_process(
    COMMAND "${AL_SHIM_FIXTURE_VENV_PYTHON}" -c "import h5py"
    RESULT_VARIABLE AL_SHIM_STAMP_VARIANT_H5PY_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if( AL_SHIM_STAMP_VARIANT_H5PY_RESULT EQUAL 0 )
    set( AL_SHIM_STAMP_VARIANT_TOOLING_AVAILABLE TRUE )
  endif()
endif()

if( NOT AL_SHIM_STAMP_VARIANT_TOOLING_AVAILABLE )
  message( STATUS
    "Not registering shim stamp-variant tests: needs imas-python-fixtures/.venv with h5py" )
else()
  set( AL_SHIM_STAMP_VARIANT_SOURCE "${AL_SHIM_FIXTURE_ROOT}/dd-3.39.0" )
  set( AL_SHIM_STAMP_ABSENT_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/stamp-absent-fixture" )
  set( AL_SHIM_STAMP_MALFORMED_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/stamp-malformed-fixture" )
  set( AL_SHIM_STAMP_MISMATCH_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/stamp-mismatch-fixture" )

  add_custom_command(
    OUTPUT
      "${AL_SHIM_STAMP_ABSENT_FIXTURE}/equilibrium.h5"
      "${AL_SHIM_STAMP_MALFORMED_FIXTURE}/equilibrium.h5"
      "${AL_SHIM_STAMP_MISMATCH_FIXTURE}/equilibrium.h5"
    COMMAND "${AL_SHIM_FIXTURE_VENV_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/derive_stamp_variant.py"
      "${AL_SHIM_STAMP_VARIANT_SOURCE}" "${AL_SHIM_STAMP_ABSENT_FIXTURE}" absent
    COMMAND "${AL_SHIM_FIXTURE_VENV_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/derive_stamp_variant.py"
      "${AL_SHIM_STAMP_VARIANT_SOURCE}" "${AL_SHIM_STAMP_MALFORMED_FIXTURE}" malformed
    COMMAND "${AL_SHIM_FIXTURE_VENV_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/derive_stamp_variant.py"
      "${AL_SHIM_STAMP_VARIANT_SOURCE}" "${AL_SHIM_STAMP_MISMATCH_FIXTURE}" mismatch
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/derive_stamp_variant.py"
      "${AL_SHIM_STAMP_VARIANT_SOURCE}/equilibrium.h5"
      "${AL_SHIM_STAMP_VARIANT_SOURCE}/master.h5"
    VERBATIM
  )
  # Named without the cpp-test-shim- prefix used everywhere else in this
  # file: this is a build target, not a registered CTest test, and `ctest
  # -N` should not suggest otherwise.
  add_custom_target( cpp-shim-stamp-fixtures ALL DEPENDS
    "${AL_SHIM_STAMP_ABSENT_FIXTURE}/equilibrium.h5"
    "${AL_SHIM_STAMP_MALFORMED_FIXTURE}/equilibrium.h5"
    "${AL_SHIM_STAMP_MISMATCH_FIXTURE}/equilibrium.h5"
  )

  # Prove the three variants keep the states F2.3 and F3.1 need distinct,
  # rather than trusting the derivation script did -- see
  # verify_stamp_variants.py's own docstring for the failure this would
  # catch.
  add_test( NAME cpp-test-shim-stamp-variants
    COMMAND "${AL_SHIM_FIXTURE_VENV_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_stamp_variants.py"
      "${AL_SHIM_STAMP_ABSENT_FIXTURE}" "${AL_SHIM_STAMP_MALFORMED_FIXTURE}" "${AL_SHIM_STAMP_MISMATCH_FIXTURE}"
  )
  set_tests_properties( cpp-test-shim-stamp-variants PROPERTIES
    LABELS "shim;harness"
    FAIL_REGULAR_EXPRESSION "STAMP-VARIANT-FAILURE"
  )

  # F3.1 (docs/SHIM_SUITE_CONVENTION.md S5.3): a present but malformed
  # DD-version stamp belongs to the occurrence, not its enclosing data entry.
  # IDS::open therefore forwards, while equilibrium.get reaches the occurrence
  # open and returns the shim's refusal before any data seam is served.
  add_dependencies( cpp-test-shim-stamp-malformed cpp-shim-stamp-fixtures )
  al_cpp_shim_private_loss_log( cpp-test-shim-stamp-malformed
    AL_SHIM_STAMP_MALFORMED_LOSS_LOG_DIR )

  # The program verifies every returned public status and the untouched IDS.
  # Its generated get() prints the frozen reason rather than returning it, so
  # the read-fixture wrapper checks program success first, then captures the
  # message from standard output. This is F3.1's named diagnostic exception;
  # a PASS_REGULAR_EXPRESSION would be unsound because it can accept matching
  # output from a program that exited unsuccessfully.
  add_test( NAME cpp-test-shim-stamp-malformed
    COMMAND ${CMAKE_COMMAND}
      -D "FIXTURE_ROOT=${AL_SHIM_STAMP_MALFORMED_FIXTURE}"
      -D "COMMAND_TO_RUN=$<TARGET_FILE:cpp-test-shim-stamp-malformed>;${AL_SHIM_STAMP_MALFORMED_FIXTURE}"
      -D "EXPECTED_STDOUT_SUBSTRING=malformed DD-version stamp"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_fixture_unchanged.cmake"
  )
  set_tests_properties( cpp-test-shim-stamp-malformed PROPERTIES
    ENVIRONMENT "${AL_CPP_SHIM_TEST_ENVIRONMENT};IMAS_MVDD_LOSS_LOG_DIR=${AL_SHIM_STAMP_MALFORMED_LOSS_LOG_DIR}"
    FIXTURES_REQUIRED cpp-test-shim-stamp-malformed-loss-log
    LABELS "shim;contract-assertion"
    FAIL_REGULAR_EXPRESSION "REFUSAL-FAILURE;SCENARIO-FAILURE"
  )

  # Family 2 (docs/SHIM_SUITE_CONVENTION.md S5.2, issue #13): all four
  # passthrough states read an HDF5 equilibrium fixture. They share one
  # binary but are four CTest commands, hence four processes; that isolation
  # matters because the shim latches an HLI DD version once per process.
  if( NOT AL_BACKEND_HDF5 )
    message( STATUS "Not registering shim passthrough scenarios: HDF5 backend is disabled" )
  else()
    add_executable( cpp-test-shim-passthrough test_shim_passthrough.cpp )
    target_include_directories( cpp-test-shim-passthrough PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
    target_link_libraries( cpp-test-shim-passthrough al-cpp )
    add_dependencies( cpp-test-shim-passthrough cpp-shim-stamp-fixtures )

    function( al_cpp_shim_add_passthrough_scenario test_name scenario fixture use_hli_version )
      al_cpp_shim_private_loss_log( ${test_name} _loss_log_dir )

      # The version-unset process gets a newly composed CTest environment that
      # contains the Core loader location but never contains the injected HLI
      # version. Do not launch it with a version and unset it in a wrapper:
      # absence, not an empty value or a late mutation, is F2.1's contract.
      set( _environment ${AL_CPP_SHIM_TEST_ENVIRONMENT}
        "IMAS_MVDD_LOSS_LOG_DIR=${_loss_log_dir}" )
      if( NOT use_hli_version )
        list( FILTER _environment EXCLUDE REGEX "^IMAS_MVDD_HLI_DD_VERSION=" )
      endif()

      add_test( NAME ${test_name}
        COMMAND ${CMAKE_COMMAND}
          -D "FIXTURE_ROOT=${fixture}"
          -D "LOSS_LOG_DIR=${_loss_log_dir}"
          -D "COMMAND_TO_RUN=$<TARGET_FILE:cpp-test-shim-passthrough>;${scenario};${fixture}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_fixture_unchanged.cmake"
      )
      set_tests_properties( ${test_name} PROPERTIES
        FIXTURES_REQUIRED "${test_name}-loss-log"
        LABELS "shim;contract-assertion"
        ENVIRONMENT "${_environment}"
        FAIL_REGULAR_EXPRESSION "PASSTHROUGH-FAILURE|SCENARIO-FAILURE"
      )
    endfunction()

    al_cpp_shim_add_passthrough_scenario(
      cpp-test-shim-version-unset version-unset
      "${AL_SHIM_FIXTURE_ROOT}/dd-3.39.0" FALSE )
    al_cpp_shim_add_passthrough_scenario(
      cpp-test-shim-stamp-equal stamp-equal
      "${AL_SHIM_FIXTURE_ROOT}/dd-4.1.1" TRUE )
    al_cpp_shim_add_passthrough_scenario(
      cpp-test-shim-stamp-absent stamp-absent
      "${AL_SHIM_STAMP_ABSENT_FIXTURE}" TRUE )
    al_cpp_shim_add_passthrough_scenario(
      cpp-test-shim-stamp-mismatch-no-artifact stamp-mismatch-no-artifact
      "${AL_SHIM_STAMP_MISMATCH_FIXTURE}" TRUE )

    # Family 4.1 (docs/SHIM_SUITE_CONVENTION.md S5.4, issue #15): compare
    # each structural conversion rule against the same equilibrium read from
    # the DD 4.1.1 oracle fixture.  The program owns the rule table and the
    # public-HLI reads; CTest supplies the immutable fixture root and private
    # loss-log directory that make the cross-version read isolated.
    add_executable( cpp-test-shim-structural-rules test_shim_structural_rules.cpp )
    target_include_directories( cpp-test-shim-structural-rules PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
    target_link_libraries( cpp-test-shim-structural-rules PRIVATE al-cpp )
    al_cpp_shim_private_loss_log( cpp-test-shim-structural-rules
      AL_SHIM_STRUCTURAL_LOSS_LOG_DIR )
    add_test( NAME cpp-test-shim-structural-rules
      COMMAND ${CMAKE_COMMAND}
        -D "FIXTURE_ROOT=${AL_SHIM_FIXTURE_ROOT}"
        -D "COMMAND_TO_RUN=$<TARGET_FILE:cpp-test-shim-structural-rules>;${AL_SHIM_FIXTURE_ROOT}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_fixture_unchanged.cmake"
    )
    set_tests_properties( cpp-test-shim-structural-rules PROPERTIES
      ENVIRONMENT "${AL_CPP_SHIM_TEST_ENVIRONMENT};IMAS_MVDD_LOSS_LOG_DIR=${AL_SHIM_STRUCTURAL_LOSS_LOG_DIR}"
      FIXTURES_REQUIRED cpp-test-shim-structural-rules-loss-log
      LABELS "shim;contract-assertion"
      FAIL_REGULAR_EXPRESSION "STRUCTURAL-FAILURE;SCENARIO-FAILURE"
    )
  endif()
endif()

# Family 6 (docs/SHIM_SUITE_CONVENTION.md S5.6, issue #21): one program
# exercises the slice-append round trip both across DD versions and against a
# same-version control. Keep registration in one parameterless function:
# these two executions are one argument, and neither may be independently
# selected out of the suite.
if( NOT AL_BACKEND_HDF5 )
  message( STATUS "Not registering shim round-trip scenarios: HDF5 backend is disabled" )
else()
  add_executable( cpp-test-shim-roundtrip test_shim_roundtrip.cpp )
  target_include_directories( cpp-test-shim-roundtrip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
  target_link_libraries( cpp-test-shim-roundtrip PRIVATE al-cpp )

  function( al_cpp_shim_add_roundtrip_pair )
    set( _cross_test cpp-test-shim-roundtrip-cross-dd )
    set( _same_test cpp-test-shim-roundtrip-same-dd )

    al_cpp_shim_private_fixture_copy( ${_cross_test}
      "${AL_SHIM_FIXTURE_ROOT}/dd-3.39.0" _cross_fixture )
    al_cpp_shim_private_fixture_copy( ${_same_test}
      "${AL_SHIM_FIXTURE_ROOT}/dd-4.1.1" _same_fixture )
    al_cpp_shim_private_loss_log( ${_cross_test} _cross_loss_log_dir )
    al_cpp_shim_private_loss_log( ${_same_test} _same_loss_log_dir )

    add_test( NAME ${_cross_test}
      COMMAND $<TARGET_FILE:cpp-test-shim-roundtrip>
        "${_cross_fixture}" partial-read-allowed
    )
    set_tests_properties( ${_cross_test} PROPERTIES
      FIXTURES_REQUIRED "${_cross_test}-fixture;${_cross_test}-loss-log"
      LABELS "shim;contract-assertion"
      ENVIRONMENT "${AL_CPP_SHIM_TEST_ENVIRONMENT};IMAS_MVDD_LOSS_LOG_DIR=${_cross_loss_log_dir}"
      FAIL_REGULAR_EXPRESSION "ROUNDTRIP-FAILURE"
    )

    add_test( NAME ${_same_test}
      COMMAND $<TARGET_FILE:cpp-test-shim-roundtrip>
        "${_same_fixture}" clean-read
    )
    set_tests_properties( ${_same_test} PROPERTIES
      FIXTURES_REQUIRED "${_same_test}-fixture;${_same_test}-loss-log"
      LABELS "shim;contract-assertion"
      ENVIRONMENT "${AL_CPP_SHIM_TEST_ENVIRONMENT};IMAS_MVDD_LOSS_LOG_DIR=${_same_loss_log_dir}"
      FAIL_REGULAR_EXPRESSION "ROUNDTRIP-FAILURE"
    )
  endfunction()

  al_cpp_shim_add_roundtrip_pair()
endif()
