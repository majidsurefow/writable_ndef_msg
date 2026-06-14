# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_PROTOCOL_ULTRALIGHT ultralight.c)
zephyr_library_sources_ifdef(CONFIG_NFC_ROLE_READER ultralight_poller.c)

if(CONFIG_NFC_PROTOCOL_ULTRALIGHT AND (CONFIG_NFC_ROLE_LISTEN OR CONFIG_NFC_STORE))
  zephyr_library_sources(ultralight_listener.c)
endif()

zephyr_library_include_directories_ifdef(CONFIG_NFC_PROTOCOL_ULTRALIGHT
  ${CMAKE_CURRENT_SOURCE_DIR}
)
