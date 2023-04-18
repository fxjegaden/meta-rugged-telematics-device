FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
	file://PATCH003-iW-PRGST-SC-R1.0-REL2.1-L5.4.70_i.MX_6ULL_Standard_uBoot_Customization.patch \
	file://PATCH004-iW-PRGST-SC-R1.0-REL2.2-L5.4.70_i.MX_6ULL_uBoot_Battery_Charger_Disabled_in_Serial_Mode.patch \
	file://PATCH005-iW-PRGST-SC-R1.0-REL2.2.1-L5.4.70_i.MX_6ULL_uBoot_Battery_Charging_Current.patch \
"
COMPATIBLE_MACHINE = "(imx6ull-iwg26i)"
