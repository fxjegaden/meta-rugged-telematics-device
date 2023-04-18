SUMMARY = "Rugged Telematics Device QGPS Test Application Compilation"
DESCRIPTION = "Recipe to compile the QGPS Test Application for Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI = "file://src file://inc"

FILES_${PN} = "${ROOT_HOME}"

PACKAGES = "${PN} ${PN}-dbg ${PN}-dev"
S = "${WORKDIR}/src/"

INSANE_SKIP_${PN} = "ldflags"
RDEPENDS_${PN} = "rtd-library-source"
DEPENDS = "rtd-library-source"

do_compile() {
        oe_runmake
}

do_install() {
        install -d ${D}${ROOT_HOME}
        install -m 755 ${S}/QGPS_APP ${D}${ROOT_HOME}
}
