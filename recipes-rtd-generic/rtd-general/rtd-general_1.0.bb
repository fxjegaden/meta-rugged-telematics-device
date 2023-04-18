SUMMARY = "Access 4G Internet Connection from SIM in Rugged Telematics Device"
DESCRIPTION = "Recipe to support access 4G Internet Connection from SIM in Rugged Telematics Device"
LICENSE = "CLOSED"

SRC_URI = "file://gprs_4g \
	file://options-mobile \
	file://gprs \
	file://udhcpd.conf \
"

do_install() {
        install -d ${D}/${sysconfdir}/bluetooth/
        install -d ${D}/${sysconfdir}/ppp/peers/
        install -d ${D}/${sysconfdir}/ppp/chat
        install -m 777 ${WORKDIR}/udhcpd.conf ${D}/${sysconfdir}/
        install -m 777 ${WORKDIR}/options-mobile ${D}/${sysconfdir}/ppp/
        install -m 777 ${WORKDIR}/gprs_4g ${D}/${sysconfdir}/ppp/peers
        install -m 777 ${WORKDIR}/gprs ${D}/${sysconfdir}/ppp/chat/
}

EXCLUDE_FROM_WORLD = "1"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
