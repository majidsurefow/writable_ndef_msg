if(CONFIG_NFC_APPLETS)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_policy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_scan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_read.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_verify_compare.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_verify.c
  )

  if(CONFIG_NFC_LISTEN_STACK)
    target_sources(app PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_emulate.c
      ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_loop.c
    )
  endif()

  if(CONFIG_NFC_APPLETS_SHELL)
    target_sources(app PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/nfc_applet_shell_cmds.c
    )
  endif()
endif()
