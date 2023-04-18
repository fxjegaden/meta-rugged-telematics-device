FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://wpa_supplicant.conf"

do_install_append() {
        install -m 0644 ${WORKDIR}/wpa_supplicant.conf ${D}${sysconfdir}
}

