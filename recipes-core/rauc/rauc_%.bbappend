FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append := " \
    file://system.conf \
"

SYSTEMD_AUTO_ENABLE_${PN}-mark-good = "disable"