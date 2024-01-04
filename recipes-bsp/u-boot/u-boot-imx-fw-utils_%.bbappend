require u-boot-imx-common_${PV}.inc

SUMMARY = "U-Boot bootloader fw_printenv/setenv utilities"
DEPENDS = "mtd-utils bison-native"

FILESEXTRAPATHS_prepend := "${THISDIR}/u-boot-imx:"

SRC_URI += " \
	file://PATCH003-iW-PRGST-SC-R1.0-REL2.1-L5.4.70_i.MX_6ULL_Standard_uBoot_Customization.patch \
	file://PATCH004-iW-PRGST-SC-R1.0-REL2.2-L5.4.70_i.MX_6ULL_uBoot_Battery_Charger_Disabled_in_Serial_Mode.patch \
	file://PATCH005-iW-PRGST-SC-R1.0-REL2.2.1-L5.4.70_i.MX_6ULL_uBoot_Battery_Charging_Current.patch \
    file://fw_env.config \
"
COMPATIBLE_MACHINE = "(imx6ull-iwg26i)"

EXTRA_OEMAKE_class-target = 'CROSS_COMPILE=${TARGET_PREFIX} CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'
EXTRA_OEMAKE_class-cross = 'ARCH=${TARGET_ARCH} CC="${CC} ${CFLAGS} ${LDFLAGS}" STRIP=true V=1'

inherit uboot-config

do_compile () {
    oe_runmake ${UBOOT_MACHINE}
    oe_runmake envtools
}

do_install () {
    install -d ${D}${base_sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 ${S}/tools/env/fw_printenv ${D}${base_sbindir}/fw_printenv
    install -m 755 ${S}/tools/env/fw_printenv ${D}${base_sbindir}/fw_setenv

    if [ -e ${WORKDIR}/fw_env.config ]; then
        install -m 0644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
    else
        install -m 0644 ${S}/tools/env/fw_env.config ${D}${sysconfdir}/fw_env.config
    fi
}

do_install_class-cross () {
    install -d ${D}${bindir_cross}
    install -m 755 ${S}/tools/env/fw_printenv ${D}${bindir_cross}/fw_printenv
    install -m 755 ${S}/tools/env/fw_printenv ${D}${bindir_cross}/fw_setenv
}

SYSROOT_PREPROCESS_FUNCS_class-cross = "uboot_fw_utils_cross"
uboot_fw_utils_cross() {
    sysroot_stage_dir ${D}${bindir_cross} ${SYSROOT_DESTDIR}${bindir_cross}
}

PROVIDES += "u-boot-fw-utils"
RPROVIDES_${PN} += "u-boot-fw-utils"

PACKAGE_ARCH = "${MACHINE_ARCH}"
BBCLASSEXTEND = "cross"
