FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
KERNEL_MODULE_INSTALL_PATH = "${WORKDIR}/image/lib/modules/${KERNEL_VERSION}/kernel"
KERNEL_MODULE_PATH = "${D}/iwtest/kernel-module/"

FILES_${KERNEL_PACKAGE_NAME}-modules += "/iwtest/kernel-module/*.ko"
SRC_URI += " \
	file://PATCH003-iW-PRGST-SC-R1.0-REL2.1-L5.4.70_i.MX_6ULL_Standard_Kernel_Customization.patch \
	file://PATCH004-iW-PRGST-SC-R1.0-REL2.2-L5.4.70_i.MX_6ULL_Timer_and_DPDM_Timeout.patch \
	file://PATCH005-iW-PRGST-SC-R1.0-REL2.3-L5.4.70_i.MX_6ULL_Kernel_USB_Drivers_Enabled.patch \
	file://PATCH006-iW-PRGST-SC-R1.0-REL2.3.1-L5.4.70_i.MX_6ULL_Kernel_TCAN_Bit_Error_and_Watermark.patch \
	file://PATCH007-iW-PRGST-SC-R1.0-REL2.4-L5.4.70_i.MX_6ULL_Kernel_TCAN_Watermark_and_Timeout.patch \
	file://PATCH009-iW-PRGST-SC-R1.0-REL2.4.1-L5.4.70_i.MX_6ULL_CAN_FD_Wakeup_Enabled.patch \
	file://PATCH010-iW-PRGST-SC-R1.0-REL2.4.1-L5.4.70_i.MX_6ULL_CAN_FD_Transmission.patch \
	file://PATCH011-iW-PRGST-SC-R1.0-REL2.4.2-L5.4.70_i.MX_6ULL_Battery_Charge_Current.patch \
	file://PATCH012-iW-PRGST-SC-R1.0-REL2.4.3-L5.4.70_i.MX_6ULL_Bluetooth_Input_Device.patch \
"

INSANE_SKIP_${KERNEL_PACKAGE_NAME}-modules += "host-user-contaminated"

# Copy the defconfig before compilation
do_copy_defconfig () {
	install -d ${B}
	mkdir -p ${B}
	cp ${S}/arch/arm/configs/${IWG26I_KERNEL_DEFCONFIG} ${B}/.config
	cp ${S}/arch/arm/configs/${IWG26I_KERNEL_DEFCONFIG} ${B}/../defconfig
}

# Add task sequence
addtask copy_defconfig after do_unpack before do_preconfigure
addtask copy_modules after do_install before do_package

# Preventing the autoloading of below modules
KERNEL_MODULE_PROBECONF += "tcan4x5x"
module_conf_tcan4x5x = "blacklist tcan4x5x"

# Copy the required kernel modules to "${KERNEL_MODULE_PATH}" path
do_copy_modules() {
	install -d ${KERNEL_MODULE_PATH}
	install -m 644 ${KERNEL_MODULE_INSTALL_PATH}/net/can/j1939/can-j1939.ko ${KERNEL_MODULE_PATH}
	install -m 644 ${KERNEL_MODULE_INSTALL_PATH}/drivers/net/can/m_can/tcan4x5x.ko ${KERNEL_MODULE_PATH}
}

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

COMPATIBLE_MACHINE = "(imx6ull-iwg26i)"
