
inherit fsl-u-boot-localversion

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/gpl-2.0.txt;md5=b234ee4d69f5fce4486a80fdaf4a4263"

UBOOT_SRC ?= "git://github.com/nxp-imx/uboot-imx.git;protocol=https"
SRCBRANCH = "imx_v2020.04_5.4.70_2.3.0"
SRC_URI = "${UBOOT_SRC};branch=${SRCBRANCH} \
"
SRCREV = "e42dee801ec55bc40347cbb98f13bfb5899f0368"


S = "${WORKDIR}/git"
PV_append = "+imx"
LOCALVERSION = "+imx"
