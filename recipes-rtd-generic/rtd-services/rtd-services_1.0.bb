SUMMARY = "Services for Rugged Telematics Device"
DESCRIPTION = "Recipe to copy Services for Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI += " \
            file://unit_files/ble.service \
            file://unit_files/restart_ble.service \
            file://unit_files/sshd.service \
            file://unit_files/watchdog.service \
            file://bins/watchdog \
            file://unit_files/wifi_enable.service \
            file://script_files/ble.sh \
            file://script_files/restart_ble_serial.sh \
            file://script_files/wifi_enable.sh \
            file://exe_files/nodered \
	   "

inherit systemd

SYSTEMD_PACKAGES = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${PN}', '', d)}"
SYSTEMD_SERVICE_${PN} = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', ' ble.service restart_ble.service sshd.service watchdog.service wifi_enable.service ', '', d)}"

do_install () {
    install -d ${D}${bindir}
    install ${WORKDIR}/script_files/*.sh ${D}${bindir}
    install ${WORKDIR}/bins/watchdog ${D}${bindir}

    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system
        install ${WORKDIR}/unit_files/*.service ${D}/${systemd_unitdir}/system
    fi

    install -d ${D}${sysconfdir}/node_red/
    install ${WORKDIR}/exe_files/nodered ${D}${sysconfdir}/node_red/
}
