# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_DESFIRE desfire.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_DESFIRE desfire_poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_DESFIRE desfire_poller_i.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_DESFIRE desfire_listener.c)

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_DESFIRE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
