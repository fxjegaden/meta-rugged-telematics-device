SUMMARY = "Packages to be added for the Yocto build with \"meta-rugged-telematics-device-source\" layer."

IMAGE_INSTALL_append += " \
	packagegroup-core-boot hostapd wpa-supplicant tzdata \
	openssl libssl usbutils can-utils iproute2 gnupg dpkg \
	python python3 python3-pip python3-pyserial ethtool \
	ppp socat gpsd udev-extraconf sudo wireless-tools \
	bluez5 cryptodev-module mmc-utils i2c-tools obexftp git \
	valgrind apt nano iw minicom iptables tcpdump wget iperf2 \
	iperf3 tar zip ntp ntpdate curl kernel-modules openobex \
	openssh nodejs nodejs-npm unzip gzip dnsmasq libwebsockets \
	rtd-modem-debug-tools rtd-firmware rtd-mcu-app-source \
	rtd-scripts rtd-services rtd-general rtd-mcu-bin \
	rtd-library-source rtd-app-source sensor-test-app-source \
	qgps-sample-app-source \
	pulseaudio-server pulseaudio-module-bluetooth-discover \
	pulseaudio-module-bluez5-device pulseaudio-module-bluez5-discover \
	bluez5-noinst-tools bluez5-obex pulseaudio phytool glibc-gconv-utf-16 \
"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image

IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE_append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "" ,d)}"

# Here we give sudo access to sudo members
update_sudoers(){
	sed -i 's/# %sudo/%sudo/' ${IMAGE_ROOTFS}/etc/sudoers
}

ROOTFS_POSTPROCESS_COMMAND += "update_sudoers;"

# Updates the Login password for the root user
inherit extrausers
EXTRA_USERS_PARAMS = "usermod -P iWavesys123 root;"
