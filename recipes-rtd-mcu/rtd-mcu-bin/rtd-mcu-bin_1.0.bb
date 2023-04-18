SUMMARY = "MCU specific Binaries for Rugged Telematics Device"
DESCRIPTION = "Recipe file for MCU binaries in Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI = "file://File_config.json \
	file://mcu_fw_updater \
	file://mcu_reset.sh \
	file://mcu_sbl_mode.service \
	file://lpcxpresso804_.bin \
	file://ISP_MODE_FW \
"

FILES_${PN} = "${ROOT_HOME} ${bindir}"

INSANE_SKIP_${PN}_append = "already-stripped ldflags"

PACKAGES = "${PN} ${PN}-dbg"
S = "${WORKDIR}"

inherit systemd

SYSTEMD_PACKAGES = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${PN}', '', d)}"
SYSTEMD_SERVICE_${PN} = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', ' mcu_sbl_mode.service ', '', d)}"

do_install() {
	install -d ${D}${ROOT_HOME}
	install -d ${D}${bindir}
	install -m 755 ${WORKDIR}/File_config.json ${D}${ROOT_HOME}
	install -m 755 ${WORKDIR}/mcu_fw_updater ${D}${ROOT_HOME}
	install -m 755 ${WORKDIR}/lpcxpresso804_.bin ${D}${ROOT_HOME}
	install -m 755 ${WORKDIR}/ISP_MODE_FW ${D}${ROOT_HOME}
	install -m 755 ${WORKDIR}/mcu_reset.sh ${D}${bindir}

	if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
		install -d ${D}${systemd_unitdir}/system
		install ${WORKDIR}/mcu_sbl_mode.service ${D}/${systemd_unitdir}/system
	fi
}
