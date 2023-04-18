SUMMARY = "Firmware files for Rugged Telematics Device"
DESCRIPTION = "Recipe for copying the Firmware files for Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI = "file://LICENCE.broadcom_bcm43xx \
    file://regulatory.db \
    file://regulatory.db.p7s \
    file://brcmfmac43455-sdio.bin \
    file://brcmfmac43455-sdio.clm_blob \
    file://brcmfmac43455-sdio.raspberrypi,3-model-b-plus.txt \
    file://brcmfmac43455-sdio.txt \
    file://epdc_E060SCM.fw \
    file://epdc_E60_V110.fw \
    file://epdc_E60_V220.fw \
    file://epdc_E97_V110.fw \
    file://epdc_ED060XH2C1.fw \
    file://BCM4345C0.1MW.hcd \
    file://sdma-imx6q.bin \
    file://sdma-imx7d.bin"

FILES_${PN} = "${base_libdir} ${sysconfdir}"

PACKAGES = "${PN} ${PN}-dbg ${PN}-dev"
S = "${WORKDIR}"

INSANE_SKIP_${PN} = "ldflags"

do_install () {
        install -d ${D}/${sysconfdir}/firmware/
        install -d ${D}/${base_libdir}/firmware/brcm/
        install -d ${D}/${base_libdir}/firmware/imx/epdc/
        install -d ${D}/${base_libdir}/firmware/imx/sdma/
        install -m 755 BCM4345C0.1MW.hcd ${D}/${sysconfdir}/firmware
        install -m 755 LICENCE.broadcom_bcm43xx ${D}/${base_libdir}/firmware
        install -m 755 regulatory.db ${D}/${base_libdir}/firmware
        install -m 755 regulatory.db.p7s ${D}/${base_libdir}/firmware
        install -m 755 brcmfmac43455-sdio.bin ${D}/${base_libdir}/firmware/brcm
        install -m 755 brcmfmac43455-sdio.clm_blob ${D}/${base_libdir}/firmware/brcm
        install -m 755 brcmfmac43455-sdio.raspberrypi,3-model-b-plus.txt ${D}/${base_libdir}/firmware/brcm
        install -m 755 brcmfmac43455-sdio.txt ${D}/${base_libdir}/firmware/brcm
        install -m 755 epdc_E060SCM.fw ${D}/${base_libdir}/firmware/imx/epdc
        install -m 755 epdc_E60_V110.fw ${D}/${base_libdir}/firmware/imx/epdc
        install -m 755 epdc_E60_V220.fw ${D}/${base_libdir}/firmware/imx/epdc
        install -m 755 epdc_E97_V110.fw ${D}/${base_libdir}/firmware/imx/epdc
        install -m 755 epdc_ED060XH2C1.fw ${D}/${base_libdir}/firmware/imx/epdc
        install -m 755 sdma-imx6q.bin ${D}/${base_libdir}/firmware/imx/sdma
        install -m 755 sdma-imx7d.bin ${D}/${base_libdir}/firmware/imx/sdma
}

EXCLUDE_FROM_WORLD = "1"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
