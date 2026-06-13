# NFC stack — gated by CONFIG_NFC_STACK.

if(CONFIG_NFC_STACK)
  add_subdirectory(hal)

  if(CONFIG_NFC_READER)
    add_subdirectory(reader)
  endif()

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
