# Propagate PN7160 devicetree overlay to the app image when sysbuild is used.
# west build -b nrf54l15dk/nrf54l15/cpuapp . \
#   -DEXTRA_CONF_FILE=overlay-pn7160-stack.conf \
#   -Dwritable_ndef_msg_DTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay
#
# When EXTRA_CONF_FILE names a PN7160 profile overlay, default the I2C shield
# overlay unless the caller already set writable_ndef_msg_DTC_OVERLAY_FILE.

if(NOT DEFINED CACHE{writable_ndef_msg_DTC_OVERLAY_FILE})
  set(_pn7160_overlay_needed FALSE)
  if(DEFINED CACHE{EXTRA_CONF_FILE})
    set(_extra_conf "${EXTRA_CONF_FILE}")
    if(_extra_conf MATCHES "overlay-pn7160")
      set(_pn7160_overlay_needed TRUE)
    endif()
  endif()
  if(_pn7160_overlay_needed)
    if(DEFINED CACHE{EXTRA_CONF_FILE} AND EXTRA_CONF_FILE MATCHES "overlay-pn7160-spi")
      set(writable_ndef_msg_DTC_OVERLAY_FILE
          "${APP_DIR}/boards/overlays/pn7160_spi.overlay"
          CACHE STRING "PN7160 SPI shield overlay for profile builds" FORCE)
    else()
      set(writable_ndef_msg_DTC_OVERLAY_FILE
          "${APP_DIR}/boards/overlays/pn7160_i2c.overlay"
          CACHE STRING "PN7160 I2C shield overlay for profile builds" FORCE)
    endif()
  endif()
  unset(_pn7160_overlay_needed)
  unset(_extra_conf)
endif()
