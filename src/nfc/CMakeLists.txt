# NFC stack — gated by CONFIG_NFC_STACK.

if(CONFIG_NFC_STACK)
  add_subdirectory(run)
  add_subdirectory(hal)

  if(CONFIG_NFC_PROTOCOL_NDEF)
    add_subdirectory(protocols/ndef)
  endif()

  if(CONFIG_NFC_PROTOCOL_ULTRALIGHT)
    add_subdirectory(protocols/ultralight)
  endif()

  if(CONFIG_NFC_PROTOCOL_CLASSIC)
    add_subdirectory(protocols/classic)
  endif()

  if(CONFIG_NFC_PROTOCOL_FELICA)
    add_subdirectory(protocols/felica)
  endif()

  if(CONFIG_NFC_STORE)
    add_subdirectory(store)
  endif()

  if(CONFIG_NFC_FRAMING)
    add_subdirectory(framing)
  endif()

  if(CONFIG_NFC_ROUTER)
    add_subdirectory(router)
  endif()

  if(CONFIG_NFC_LISTEN_STACK)
    add_subdirectory(nfc_stack)
  endif()

  if(CONFIG_NFC_READER)
    add_subdirectory(reader)
  endif()

  if(CONFIG_NFC_APPLETS)
    add_subdirectory(applets)
  endif()

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
