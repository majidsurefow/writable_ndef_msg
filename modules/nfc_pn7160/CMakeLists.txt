# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(src/pn7160_driver.c)
zephyr_library_sources(src/pn7160_tml_i2c.c)
zephyr_library_sources(src/pn7160_tml_spi.c)
zephyr_library_sources(src/pn7160_nci.c)
