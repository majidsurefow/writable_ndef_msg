# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_SLIX slix.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_SLIX slix_poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_SLIX slix_poller_i.c)

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_SLIX
  ${CMAKE_CURRENT_SOURCE_DIR}
)
