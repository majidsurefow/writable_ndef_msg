# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_LISTEN_STACK nfc_stack.c)
zephyr_library_include_directories_ifdef(CONFIG_NFC_LISTEN_STACK ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(CONFIG_NFC_LISTEN_STACK_SHELL)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/nfc_stack_shell_cmds.c
  )
endif()
