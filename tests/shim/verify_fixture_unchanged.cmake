# Run a read scenario and prove it left FIXTURE_ROOT unchanged afterwards.
#
# This is the generic helper docs/SHIM_SUITE_CONVENTION.md S3.3 asks every
# read scenario to use, wrapping the scenario's own executable the same way
# check_shim_linkage.cmake wraps nothing and check_verdict_orientation.cmake
# wraps a source scan -- the wrapping is the check.
#
# COMMAND_TO_RUN is a `;`-separated command (see cpp-test-shim-fixture-digest
# below for how CMake reconstructs it into a list); FIXTURE_ROOT is the
# directory the scenario reads from and MUST NOT write to. A scenario may
# additionally give EXPECTED_STDOUT_SUBSTRING when its only public route to a
# diagnostic is the HLI's standard output. The program's exit status is always
# checked before that captured output, so similar text from an ordinary failure
# cannot satisfy the diagnostic assertion.
if( NOT DEFINED FIXTURE_ROOT OR NOT DEFINED COMMAND_TO_RUN )
  message( FATAL_ERROR "SCENARIO-FAILURE: FIXTURE_ROOT and COMMAND_TO_RUN are required" )
endif()

include( "${CMAKE_CURRENT_LIST_DIR}/fixture_digest.cmake" )

al_cpp_shim_fixture_digest( "${FIXTURE_ROOT}" _al_cpp_shim_digest_before )

if( DEFINED EXPECTED_STDOUT_SUBSTRING )
  execute_process(
    COMMAND ${COMMAND_TO_RUN}
    RESULT_VARIABLE _al_cpp_shim_digest_result
    OUTPUT_VARIABLE _al_cpp_shim_program_stdout
    ERROR_VARIABLE _al_cpp_shim_program_stderr
  )
else()
  execute_process(
    COMMAND ${COMMAND_TO_RUN}
    RESULT_VARIABLE _al_cpp_shim_digest_result
  )
endif()

al_cpp_shim_fixture_digest( "${FIXTURE_ROOT}" _al_cpp_shim_digest_after )
if( NOT _al_cpp_shim_digest_after STREQUAL _al_cpp_shim_digest_before )
  message( SEND_ERROR "SCENARIO-FAILURE: the read modified the checked-in fixture under ${FIXTURE_ROOT}" )
endif()

if( NOT _al_cpp_shim_digest_result EQUAL 0 )
  message( FATAL_ERROR "SCENARIO-FAILURE: command exited with status ${_al_cpp_shim_digest_result}: ${COMMAND_TO_RUN}" )
endif()

if( DEFINED EXPECTED_STDOUT_SUBSTRING )
  string( FIND "${_al_cpp_shim_program_stdout}" "${EXPECTED_STDOUT_SUBSTRING}"
    _al_cpp_shim_expected_stdout_at )
  if( _al_cpp_shim_expected_stdout_at EQUAL -1 )
    message( FATAL_ERROR
      "SCENARIO-FAILURE: program standard output did not contain '${EXPECTED_STDOUT_SUBSTRING}'" )
  endif()
endif()
