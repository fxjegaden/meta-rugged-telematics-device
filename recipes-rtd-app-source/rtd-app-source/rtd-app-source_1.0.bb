SUMMARY = "Rugged Telematics Device Sample Application"
DESCRIPTION = "Recipe for Rugged Telematics Device Sample Application Compilation"
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
        install -d ${D}${ROOT_HOME}/
        install -m 755 sample_application ${D}${ROOT_HOME}/
}
