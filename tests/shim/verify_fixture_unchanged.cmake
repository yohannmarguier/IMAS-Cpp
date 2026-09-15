# Run a read scenario and prove it left FIXTURE_ROOT unchanged afterwards.
#
# This is the generic helper docs/SHIM_SUITE_CONVENTION.md S3.3 asks every
# read scenario to use, wrapping the scenario's own executable the same way
# check_shim_linkage.cmake wraps nothing and check_verdict_orientation.cmake
# wraps a source scan -- the wrapping is the check.
#
# COMMAND_TO_RUN is a `;`-separated command (see cpp-test-shim-fixture-digest
# below for how CMake reconstructs it into a list); FIXTURE_ROOT is the
# directory the scenario reads from and MUST NOT write to. A caller that
# supplies LOSS_LOG_DIR also gets the required assertion that its private
# loss-log directory stayed empty.
if( NOT DEFINED FIXTURE_ROOT OR NOT DEFINED COMMAND_TO_RUN )
  message( FATAL_ERROR "SCENARIO-FAILURE: FIXTURE_ROOT and COMMAND_TO_RUN are required" )
endif()

include( "${CMAKE_CURRENT_LIST_DIR}/fixture_digest.cmake" )

al_cpp_shim_fixture_digest( "${FIXTURE_ROOT}" _al_cpp_shim_digest_before )

execute_process(
  COMMAND ${COMMAND_TO_RUN}
  RESULT_VARIABLE _al_cpp_shim_digest_result
)

al_cpp_shim_fixture_digest( "${FIXTURE_ROOT}" _al_cpp_shim_digest_after )
if( NOT _al_cpp_shim_digest_after STREQUAL _al_cpp_shim_digest_before )
  message( SEND_ERROR "SCENARIO-FAILURE: the read modified the checked-in fixture under ${FIXTURE_ROOT}" )
endif()

if( DEFINED LOSS_LOG_DIR )
  if( NOT IS_DIRECTORY "${LOSS_LOG_DIR}" )
    message( SEND_ERROR "SCENARIO-FAILURE: private loss-log directory is missing: ${LOSS_LOG_DIR}" )
  else()
    file( GLOB _al_cpp_shim_loss_logs "${LOSS_LOG_DIR}/*" )
    if( _al_cpp_shim_loss_logs )
      message( SEND_ERROR "SCENARIO-FAILURE: expected no loss-log files under ${LOSS_LOG_DIR}" )
    endif()
  endif()
endif()

if( NOT _al_cpp_shim_digest_result EQUAL 0 )
  message( FATAL_ERROR "SCENARIO-FAILURE: command exited with status ${_al_cpp_shim_digest_result}: ${COMMAND_TO_RUN}" )
endif()
