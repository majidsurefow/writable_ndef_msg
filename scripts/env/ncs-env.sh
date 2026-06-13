#!/usr/bin/env bash
# NCS v3.2.4 environment for macOS (Nordic Toolchain Manager).
# Usage: source /path/to/writable_ndef_msg/scripts/env/ncs-env.sh
#
# Override before sourcing if your install differs:
#   export NCS_ROOT=/opt/nordic/ncs/v3.2.4
#   export NCS_TOOLCHAIN=/opt/nordic/ncs/toolchains/<bundle_id>
#
# Find bundle_id for your NCS version:
#   python3 -c "import json; print(json.load(open('/opt/nordic/ncs/toolchains/toolchains.json'))[0])" \
#     | grep -A2 v3.2.4
# Or: find /opt/nordic/ncs/toolchains -name west -path '*/bin/west' 2>/dev/null

_ncs_env_fail() {
	echo "ncs-env.sh: $*" >&2
	return 1 2>/dev/null || exit 1
}

NCS_ROOT="${NCS_ROOT:-/opt/nordic/ncs/v3.2.4}"
NCS_TOOLCHAIN="${NCS_TOOLCHAIN:-/opt/nordic/ncs/toolchains/185bb0e3b6}"

[[ -d "${NCS_ROOT}/zephyr" ]] || _ncs_env_fail "missing zephyr at NCS_ROOT=${NCS_ROOT}"
[[ -x "${NCS_TOOLCHAIN}/bin/west" ]] || _ncs_env_fail "missing west at NCS_TOOLCHAIN=${NCS_TOOLCHAIN}"

export ZEPHYR_BASE="${NCS_ROOT}/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR="${NCS_TOOLCHAIN}/opt/zephyr-sdk"
export PATH="${NCS_TOOLCHAIN}/bin:${NCS_TOOLCHAIN}/opt/zephyr-sdk/arm-zephyr-eabi/bin:${PATH}"

unset -f _ncs_env_fail
