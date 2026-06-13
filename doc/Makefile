# SPDX-License-Identifier: Apache-2.0

DOC_SRC := ../modules/nfc_pn7160/include
BUILD_DIR := _build
DOXYGEN_OUT := $(BUILD_DIR)/doxygen
COVERAGE_JSON := $(BUILD_DIR)/doc-coverage.json

.PHONY: doxygen html doxygen-coverage-json clean

doxygen:
	doxygen Doxyfile

html: doxygen
	@echo "HTML documentation: $(DOXYGEN_OUT)/html/index.html"

doxygen-coverage-json: doxygen
	python3 -m coverxygen \
		--xml-dir $(DOXYGEN_OUT)/xml \
		--src-dir $(DOC_SRC) \
		--format json-v3 \
		--output $(COVERAGE_JSON)
	@test -s $(COVERAGE_JSON)

clean:
	rm -rf $(BUILD_DIR)
