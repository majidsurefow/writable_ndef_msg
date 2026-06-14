# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_STORE nfc_store.c nfc_persist_name.c)
zephyr_library_sources_ifdef(CONFIG_NFC_STORE_RAM nfc_store_ram.c)
zephyr_library_sources_ifdef(CONFIG_NFC_STORE_RAM_SHELL nfc_store_shell_cmds.c)
zephyr_library_sources_ifdef(CONFIG_NFC_STORE_GOLDENS nfc_store_golden.c)
zephyr_library_include_directories_ifdef(CONFIG_NFC_STORE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(CONFIG_NFC_STORE_GOLDENS)
  get_filename_component(NFC_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../.. ABSOLUTE)
  zephyr_library_include_directories(${NFC_REPO_ROOT}/tests/fixtures/store)
endif()
