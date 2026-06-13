# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_NFC_ROUTER aid_router.c)
zephyr_library_include_directories_ifdef(CONFIG_NFC_ROUTER ${CMAKE_CURRENT_SOURCE_DIR}/..)
