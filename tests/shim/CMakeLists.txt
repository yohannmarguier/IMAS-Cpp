# The shim conformance suite scaffold (issue #10).
#
# The linkage test below is this suite's first, and its only content until
# later tickets (issues #11-14, #24) add the contract-assertion and
# behaviour-pin families from docs/SHIM_SUITE_CONVENTION.md S5. Until linkage
# passes, nothing else the suite will ever say means anything: every
# functional test in this repository's own suite passes just as well against
# a build that resolved its symbols straight to IMAS-Core and converted
# nothing. See tests/shim/README.md for what is and is not covered so far.
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

# Give one test a private, empty loss-log directory.
#
# Leaving IMAS_MVDD_LOSS_LOG_DIR unset is not neutral: docs/
# SHIM_INTEGRATION_CONTRACT.md section 7 says the shim then writes into the
# current working directory, which for a ctest run is the build tree, where
# the file is left for whichever test globs there next. Every test whose
# read can be non-exact therefore needs a directory of its own -- see
# docs/SHIM_SUITE_CONVENTION.md S3.4 -- whether or not it asserts on it.
#
# Neither test registered above calls into the shim's runtime (linkage only
# inspects the built library; run-guard makes no AL call), so nothing here
# calls this function yet. It is part of this ticket's scaffold anyway: the
# convention sets this rule "once, relied on by every later ticket", and the
# contract-assertion and behaviour-pin families (issues #11-14, #24) need it
# from their first test.
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
