# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_CLASSIC classic.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_CLASSIC classic_i.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_CLASSIC crypto1.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_CLASSIC iso14443_crc.c)
zephyr_library_sources_ifdef(CONFIG_NFC_ROLE_READER classic_poller.c)

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_CLASSIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
