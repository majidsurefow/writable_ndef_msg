#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(writable_ndef_msg)

FILE(GLOB app_sources src/*.c)
list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/nfc_emulation.c)
# NORDIC SDK APP START
target_sources(app  PRIVATE ${app_sources})
# NORDIC SDK APP END

if(NOT CONFIG_NFC_STACK AND CONFIG_NFC_T4T_NRFXLIB)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/nfc_emulation.c)
endif()

if(CONFIG_NFC_STACK)
  add_subdirectory(src/nfc)
endif()
