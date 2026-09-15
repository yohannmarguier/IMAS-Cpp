# Self-test of fixture_digest.cmake / verify_fixture_unchanged.cmake, from a
# synthetic directory -- no pulse, no HLI, no shim, for the same reason
# check_shim_run_guard and the comparator test themselves from literals: a
# pulse-based test can never prove its own harness right.
#
# Three properties, none of which the type system enforces:
#   1. a command that leaves the fixture alone passes;
#   2. a command that mutates a file under the fixture fails, and specifically
#      via SCENARIO-FAILURE, not by crashing some other way;
#   3. an empty fixture directory is refused outright, so "nothing changed"
#      can never be reported by having nothing to compare.
include( "${CMAKE_CURRENT_LIST_DIR}/fixture_digest.cmake" )

set( _work_dir "${CMAKE_CURRENT_BINARY_DIR}/fixture-digest-self-test" )
file( REMOVE_RECURSE "${_work_dir}" )
file( MAKE_DIRECTORY "${_work_dir}/fixture" )
file( WRITE "${_work_dir}/fixture/leaf.txt" "original\n" )

# --- property 3: vacuous check on an empty directory is refused -----------
file( MAKE_DIRECTORY "${_work_dir}/empty" )
execute_process(
  COMMAND "${CMAKE_COMMAND}" -D "FIXTURE_ROOT=${_work_dir}/empty"
    -D "COMMAND_TO_RUN=${CMAKE_COMMAND};-E;true"
    -P "${CMAKE_CURRENT_LIST_DIR}/verify_fixture_unchanged.cmake"
  RESULT_VARIABLE _empty_result
  OUTPUT_QUIET ERROR_QUIET
)
if( _empty_result EQUAL 0 )
  message( FATAL_ERROR "FIXTURE-DIGEST-FAILURE: an empty fixture directory must not pass the unchanged check vacuously" )
endif()

# --- property 1: an unmodifying command passes ------------------------------
execute_process(
  COMMAND "${CMAKE_COMMAND}" -D "FIXTURE_ROOT=${_work_dir}/fixture"
    -D "COMMAND_TO_RUN=${CMAKE_COMMAND};-E;true"
    -P "${CMAKE_CURRENT_LIST_DIR}/verify_fixture_unchanged.cmake"
  RESULT_VARIABLE _clean_result
  OUTPUT_VARIABLE _clean_output ERROR_VARIABLE _clean_output
)
if( NOT _clean_result EQUAL 0 )
  message( FATAL_ERROR "FIXTURE-DIGEST-FAILURE: a read that left the fixture alone was reported as changed:\n${_clean_output}" )
endif()

# --- property 2: a mutating command fails, distinctively -------------------
# The mutation must happen *inside* the wrapped command, between
# verify_fixture_unchanged.cmake's own before/after snapshots -- mutating the
# fixture first and then calling it would only prove the checker can compute
# a digest, since nothing would change during its own execute_process call.
file( WRITE "${_work_dir}/mutated-source.txt" "mutated\n" )
execute_process(
  COMMAND "${CMAKE_COMMAND}" -D "FIXTURE_ROOT=${_work_dir}/fixture"
    -D "COMMAND_TO_RUN=${CMAKE_COMMAND};-E;copy;${_work_dir}/mutated-source.txt;${_work_dir}/fixture/leaf.txt"
    -P "${CMAKE_CURRENT_LIST_DIR}/verify_fixture_unchanged.cmake"
  RESULT_VARIABLE _dirty_result
  OUTPUT_VARIABLE _dirty_output ERROR_VARIABLE _dirty_output
)
if( _dirty_result EQUAL 0 )
  message( FATAL_ERROR "FIXTURE-DIGEST-FAILURE: a fixture mutated between digests was reported as unchanged" )
endif()
if( NOT _dirty_output MATCHES "SCENARIO-FAILURE" )
  message( FATAL_ERROR "FIXTURE-DIGEST-FAILURE: expected a SCENARIO-FAILURE marker, got:\n${_dirty_output}" )
endif()

file( REMOVE_RECURSE "${_work_dir}" )
message( STATUS "cpp-test-shim-fixture-digest: 3/3 properties held" )
