# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

if(CONFIG_PN7160)
zephyr_library()
zephyr_library_sources(src/pn7160_driver.c)
zephyr_library_sources(src/pn7160_tml_framing.c)
zephyr_library_sources(src/pn7160_nci_parse.c)
zephyr_library_sources(src/pn7160_nci.c)
zephyr_library_sources(src/pn7160_nci_settings.c)
zephyr_library_sources(src/pn7160_nci_discovery.c)
if(CONFIG_PN7160_TML_I2C)
zephyr_library_sources(src/pn7160_tml_i2c.c)
endif()
if(CONFIG_PN7160_TML_SPI)
zephyr_library_sources(src/pn7160_tml_spi.c)
endif()
if(CONFIG_PN7160_SHELL)
zephyr_library_sources(src/pn7160_shell.c)
endif()
endif()
