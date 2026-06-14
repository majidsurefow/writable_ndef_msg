# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_EMV emv.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_EMV emv_poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_EMV emv_listener.c)

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_EMV
  ${CMAKE_CURRENT_SOURCE_DIR}
)
