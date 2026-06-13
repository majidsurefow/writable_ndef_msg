if(CONFIG_NFC_STACK)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_stack_workq.c
  )
endif()
