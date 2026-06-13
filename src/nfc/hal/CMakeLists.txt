if(CONFIG_NFC_HAL_BACKEND_PN7160)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nfc_transport_pn7160.c)
endif()

if(CONFIG_NFC_HAL_BACKEND_NRFX)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nfc_transport_nrfx.c)
endif()

if(CONFIG_NFC_HAL_SHELL)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nfc_transport_shell_cmds.c)
endif()
