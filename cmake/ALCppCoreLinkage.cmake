# Core is still acquired for its C++ headers and for the shim to load at runtime.
# Do not propagate its link interface in shim mode: linking both libraries can
# bind the mirrored C ABI symbols directly to Core and bypass conversion.
set( AL_CPP_CORE_TARGET al )
set( AL_CPP_CORE_PC_REQUIRES "al-core >= ${AL_CORE_VERSION}" )
set( AL_CPP_CORE_PC_CFLAGS "" )

if( AL_USE_MULTIVERSION_SHIM )
  find_package( imas-mvdd-loader REQUIRED CONFIG )
  set( AL_CPP_CORE_TARGET imas-mvdd-loader::imas-mvdd-loader )
  set( AL_CPP_CORE_PC_REQUIRES "imas-mvdd-loader" )
  target_include_directories( al-cpp PUBLIC
    "$<BUILD_INTERFACE:$<TARGET_PROPERTY:al,INTERFACE_INCLUDE_DIRECTORIES>>"
  )

  if( AL_DOWNLOAD_DEPENDENCIES OR AL_DEVELOPMENT_LAYOUT )
    # Build Core for runtime use even when building only the al-cpp target.
    # Its headers are installed alongside the C++ headers by the Core subproject.
    add_dependencies( al-cpp al )
  else()
    # Installed Core may live in a different prefix. Export only its compiler
    # flags, not its libraries, to downstream pkg-config consumers.
    string( JOIN " " AL_CPP_CORE_PC_CFLAGS ${al_CFLAGS} )
  endif()

  message( STATUS "C++ Access Layer calls use the multiversion shim (${imas-mvdd-loader_DIR})" )
endif()
