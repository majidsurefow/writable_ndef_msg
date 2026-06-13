if(CONFIG_NFC_READER)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_reader_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_reader_poller_registry.c
  )

  if(CONFIG_NFC_READER_SHELL)
    target_sources(app PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/nfc_reader_shell_cmds.c
    )
  endif()
endif()
