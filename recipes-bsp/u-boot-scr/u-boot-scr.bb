SUMMARY = "U-boot boot scripts for RTU iwave"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

COMPATIBLE_MACHINE = "(imx6ull-iwg26i)"

DEPENDS = "u-boot-mkimage-native u-boot-mkenvimage-native"

#INHIBIT_DEFAULT_DEPS = "1"

SRC_URI = "file://uboot-env.txt"

do_compile() {
    mkenvimage -s 0x4000 -o uboot-env.bin "${THISDIR}/files/uboot-env.txt"
    mkimage -A ${UBOOT_ARCH} -T script -C none -n "Boot script" -d "${THISDIR}/files/boot.sh" boot.scr
}
inherit kernel-arch deploy

do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 0644 uboot-env.bin ${DEPLOYDIR}
}

addtask do_deploy after do_compile
#addtask do_install after do_deploy 

PROVIDES += "u-boot-default-script"

do_install() {
	install -d ${D}/boot
	install -m 0644 boot.scr ${D}/boot
	install -d ${D}/boot
	install -m 0644 uboot-env.bin ${D}/boot
}

FILES_${PN} += "/boot/boot.scr"
FILES_${PN} += "/boot/uboot-env.bin"