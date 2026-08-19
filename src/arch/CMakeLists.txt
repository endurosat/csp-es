if(CSP_POSIX)
  add_subdirectory(posix)
  if(NOT HAVE_ENDIAN_H)
    # No <endian.h> on this POSIX platform (e.g. Darwin): csp/arch/csp_endian.h only
    # declares htobeNN/beNNtoh/etc in that case, so provide the portable implementation
    # already used for Windows/FreeRTOS builds.
    add_subdirectory(std)
  endif()
elseif(CSP_FREERTOS)
  add_subdirectory(freertos)
  add_subdirectory(std)
elseif(CSP_ZEPHYR)
  add_subdirectory(zephyr)
elseif(CSP_WINDOWS)
  add_subdirectory(windows)
  add_subdirectory(std)
endif()
