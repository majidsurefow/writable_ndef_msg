/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 shell commands — probe and firmware version.
 */

#include <nfc/pn7160.h>

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

static const struct device *pn7160_shell_dev(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_pn7160)
	return DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pn7160));
#else
	return NULL;
#endif
}

static int cmd_pn7160_probe(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	dev = pn7160_shell_dev();
	if (dev == NULL) {
		shell_error(sh, "No PN7160 instance in devicetree");
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "PN7160 device not ready");
		return -ENODEV;
	}

	ret = pn7160_nci_check_dev_pres(dev);
	if (ret != 0) {
		shell_error(sh, "CORE_RESET probe failed: %d", ret);
		return ret;
	}

	shell_print(sh, "PN7160 present (CORE_RESET OK)");
	return 0;
}

static int cmd_pn7160_fwver(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	const uint8_t *fw;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	dev = pn7160_shell_dev();
	if (dev == NULL) {
		shell_error(sh, "No PN7160 instance in devicetree");
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "PN7160 device not ready");
		return -ENODEV;
	}

	fw = pn7160_fw_version_get(dev);
	if (fw[0] == 0U && fw[1] == 0U && fw[2] == 0U) {
		ret = pn7160_nci_check_dev_pres(dev);
		if (ret != 0) {
			shell_error(sh, "Probe failed: %d", ret);
			return ret;
		}
		fw = pn7160_fw_version_get(dev);
	}

	shell_print(sh, "FW version: %02x.%02x.%02x", fw[0], fw[1], fw[2]);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pn7160_cmds,
			       SHELL_CMD(probe, NULL, "CORE_RESET presence probe", cmd_pn7160_probe),
			       SHELL_CMD(fwver, NULL, "Read firmware version from CORE_RESET_NTF",
					 cmd_pn7160_fwver),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pn7160, &pn7160_cmds, "PN7160 NFC controller commands", NULL);
