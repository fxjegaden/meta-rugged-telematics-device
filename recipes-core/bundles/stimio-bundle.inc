SUMMARY = "Stimio bundles"

#DEPENDS += "phytec-dev-ca-native"
#do_bundle[depends] += "phytec-dev-ca-native:do_install"

inherit bundle

RAUC_BUNDLE_DESCRIPTION ?= "Stimio bundle"

RAUC_BUNDLE_COMPATIBLE ?= "${MACHINE}"

RAUC_BUNDLE_SLOTS ?= " \
    rootfs \
"

RAUC_SLOT_rootfs[type] ?= "image"
RAUC_IMAGE_FSTYPE ?= "ext4"


RAUC_KEY_FILE ?= "${THISDIR}/files/key.pem"
RAUC_CERT_FILE ?= "${THISDIR}/files/ca.cert.pem"
