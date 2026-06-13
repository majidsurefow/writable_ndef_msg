.. _pn7160_driver:

NXP PN7160 NFC Controller
#########################

Overview
========

The **NXP PN7160** is an NFC controller IC that exposes an **NCI 2.0** host
interface over a Transport Mapping Layer (TML) on **I2C** or **SPI**. The
Zephyr driver lives out-of-tree in ``modules/nfc_pn7160/`` and follows the same
device-model patterns as upstream sensor drivers (ENS160, BMM150).

The driver provides:

* Devicetree instantiation with ``nxp,pn7160`` (I2C or SPI bus child)
* GPIO control for **HOST_IRQ**, **VEN**, and optional **DWL_REQ**
* TML framing (NCI header parse, bus mutex, send retry)
* NCI bring-up: ``CORE_RESET`` probe and firmware version from ``CORE_RESET_NTF``
* IRQ-deferred receive on a module work queue (no bus I/O in ISR)

Higher-level discovery, NDEF read/write, and card emulation are **Phase 1+**
and live in the NFC stack, not in the low-level driver.

.. note::

   This document is a **draft** for a future upstream submission at
   ``zephyr/doc/drivers/nfc/pn7160.rst``. Until then, the authoritative
   integration spec is
   ``docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md``.

Configuration
=============

Kconfig
-------

:zephyr:code:`CONFIG_PN7160` enables the driver when a devicetree instance is
present. Transport backends are auto-selected:

* :zephyr:code:`CONFIG_PN7160_TML_I2C` — instance on I2C
* :zephyr:code:`CONFIG_PN7160_TML_SPI` — instance on SPI

Optional :zephyr:code:`CONFIG_PN7160_SHELL` registers ``pn7160 probe``,
``pn7160 fwver``, ``pn7160 init``, ``pn7160 reset``, ``pn7160 xcv``, and
``pn7160 dwl`` shell commands.

Devicetree
----------

The PN7160 node uses compatible ``nxp,pn7160`` and must be a child of exactly
one bus controller (I2C **or** SPI, not both).

**Required properties**

* ``reg`` — I2C address (typically ``0x28``) or SPI chip-select index
* ``irq-gpios`` — HOST_IRQ, active high while payload is available
* ``ven-gpios`` — enable / hard-reset, active high

**Optional properties**

* ``dwl-gpios`` — firmware download request (active high)

**Example — I2C (nRF54L15 DK + PN7160 eval shield)**

.. code-block:: devicetree

   &i2c21 {
       pn7160: pn7160@28 {
           compatible = "nxp,pn7160";
           reg = <0x28>;
           irq-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
           ven-gpios = <&gpio1 11 GPIO_ACTIVE_HIGH>;
       };
   };

**Example — SPI**

.. code-block:: devicetree

   &spi21 {
       pn7160: pn7160@0 {
           compatible = "nxp,pn7160";
           reg = <0>;
           spi-max-frequency = <500000>;
           irq-gpios = <&gpio1 10 GPIO_ACTIVE_HIGH>;
           ven-gpios = <&gpio1 11 GPIO_ACTIVE_HIGH>;
       };
   };

See ``boards/overlays/pn7160_i2c.overlay`` and ``pn7160_spi.overlay`` in the
application repository.

API reference
=============

.. doxygengroup:: pn7160_api

.. note::

   Doxygen groups will be added to ``include/nfc/pn7160.h`` before upstream
   submission. Current public symbols:

* :c:func:`pn7160_reset`
* :c:func:`pn7160_dwl_enter`
* :c:func:`pn7160_dwl_leave`
* :c:func:`pn7160_dwl_mode_get`
* :c:func:`pn7160_wait_irq`
* :c:func:`pn7160_nci_check_dev_pres`
* :c:func:`pn7160_nci_connect`
* :c:func:`pn7160_fw_version_get`
* :c:func:`pn7160_nci_transceive`
* :c:func:`pn7160_nci_host_transceive`
* :c:func:`pn7160_nci_core_reset_rsp_validate`
* :c:func:`pn7160_nci_core_reset_ntf_fw_version`

Usage
=====

Bring-up sequence
-----------------

1. Ensure the devicetree node is ``status = "okay"``.
2. Call :c:func:`device_is_ready` on the PN7160 device.
3. Call :c:func:`pn7160_nci_check_dev_pres` to send ``CORE_RESET`` and cache
   firmware version from ``CORE_RESET_NTF``.
4. Call :c:func:`pn7160_nci_connect` (or ``pn7160 init`` shell) for ``CORE_INIT``.
5. (Phase 0.9+) Apply ``ConfigureSettings`` before discovery.

Shell (development)
-------------------

.. code-block:: console

   uart:~$ pn7160 probe
   PN7160 present (CORE_RESET OK)
   uart:~$ pn7160 init
   PN7160 connected (CORE_INIT OK)
   uart:~$ pn7160 fwver
   FW version: 12.01.05
   uart:~$ pn7160 dwl enter
   DWL mode entered (TML framing active)
   uart:~$ pn7160 xcv 20 00 01 01
   RX (4): 40 00 03 00

Implementation notes
====================

TML framing
-----------

Matches NXP ``source/TML/tml.c`` (SW6705 / AN13288):

* **I2C:** raw write; read 3-byte NCI prefix then payload
* **SPI:** ``0x7F`` + payload write; ``0xFF`` dummy + payload read; 10 µs
  post-read delay
* **Send retry:** one 10 ms backoff retry on bus failure (``tml_Tx``)

VEN reset
---------

``pn7160_reset()`` drives VEN low 10 ms, then high 10 ms (``tml_Reset``).

Firmware download (DWL)
-----------------------

:c:func:`pn7160_dwl_enter` and :c:func:`pn7160_dwl_leave` port NXP
``tml_EnterDwlMode`` / ``tml_LeaveDwlMode``: assert or deassert **DWL_REQ**,
toggle download TML framing (1-byte header, 2-byte footer), and apply the VEN
reset sequence. Requires ``dwl-gpios`` in devicetree.

IRQ model
---------

HOST_IRQ edge-to-active triggers a GPIO ISR that sets an atomic flag and
submits work. TML receive runs on the module work queue and signals a
semaphore consumed by :c:func:`pn7160_nci_transceive`. No I2C/SPI traffic
occurs in the ISR.

NXP port attribution
--------------------

Logic ported from NXP-NCI2.0 MCUXpresso examples (SW6705) retains NXP
Semiconductors copyright headers in ``pn7160_tml_*.c`` and ``pn7160_nci.c``.
Reference tree (not vendored): ``hals_temp/NXP-NCI2.0_LPC55S6x_examples/``.

Related documents
=================

* Module README: ``modules/nfc_pn7160/README.md``
* Architecture spec: ``docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md``
* NXP library audit: ``docs/nfc/specs/2026-06-14-pn7160-nxp-library-audit.md``
* Shell / examples map: ``docs/nfc/PN7160_SHELL_AND_EXAMPLES.md``
