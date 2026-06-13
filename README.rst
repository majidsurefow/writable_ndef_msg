.. _writable_ndef_msg:

NFC: Type 4 tag emulation
#########################

.. contents::
   :local:
   :depth: 2

This sample runs Nordic **Type 4 Tag** emulation with a **chosen NFCID1 (UID)** and a small **in-RAM NDEF file** (empty placeholder record). Nothing is stored in flash.

Requirements
************

.. table-from-sample-yaml::

Overview
********

On boot the app calls ``nfc_on()`` (see ``nfc_emulation.c``): ``nfc_t4t_setup``, NFCID1, ``nfc_t4t_ndef_rwpayload_set``, then ``nfc_t4t_emulation_start``.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      * **LED 1:** NFC field detected.
      * **LED 4:** NDEF read (last byte of NDEF data touched).

   .. group-tab:: nRF54 DKs

      * **LED 0:** NFC field detected.
      * **LED 3:** NDEF read.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/writable_ndef_msg`

.. include:: /includes/build_and_run_ns.txt

Dependencies
************

* Type 4 Tag library (``sdk-nrfxlib``)
* DK library for LEDs
