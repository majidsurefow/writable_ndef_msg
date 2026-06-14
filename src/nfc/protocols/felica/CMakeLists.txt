# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_FELICA felica.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_FELICA felica_poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_FELICA felica_poller_i.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_FELICA felica_listener.c)
zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_FELICA
  ${CMAKE_CURRENT_SOURCE_DIR}/../../helpers/felica_crc.c
)

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_FELICA
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
