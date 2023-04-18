SUMMARY = "Scripts for Rugged Telematics Device"
DESCRIPTION = "Recipe to copy the scripts for Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI = "file://4g_on.sh \
	   file://4g_ping_test.sh \
	   file://ble.sh \
	   file://export.sh \
	   file://switch_on.sh \
	   file://wakeup_conf.sh "

S = "${WORKDIR}"

FILES_${PN} = "${ROOT_HOME}"

do_install() {
        install -d ${D}${ROOT_HOME}
        install -m 755 ${S}/*.sh ${D}${ROOT_HOME}
}

