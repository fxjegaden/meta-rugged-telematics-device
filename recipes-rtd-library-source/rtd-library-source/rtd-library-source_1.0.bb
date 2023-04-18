SUMMARY = "LibRugged Telematics Library for Rugged Telematics Device"
DESCRIPTION = "Recipe to compile the LibRugged Telematics Library for Rugged Telematics Gateway"
LICENSE = "CLOSED"

SRC_URI = "file://src file://inc"

FILES_${PN} = "${libdir}"

PACKAGES = "${PN} ${PN}-dbg"
S = "${WORKDIR}/src"

INSANE_SKIP_${PN} = "ldflags"

do_compile() {
        oe_runmake
}

do_install() {
        install -d ${D}${libdir}
        install -m 755 libRuggedTelematics.so ${D}${libdir}
}
