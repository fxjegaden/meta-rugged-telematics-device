
echo "attemps slot A:${BOOT_B_LEFT} attempts remaining"
echo "attemps slot B:${BOOT_B_LEFT} attempts remaining"
test -n "${BOOT_ORDER}" || setenv BOOT_ORDER "A B"
test -n "${BOOT_A_LEFT}" || setenv BOOT_A_LEFT 3
test -n "${BOOT_B_LEFT}" || setenv BOOT_B_LEFT 3

setenv default_bootargs "console=${console},${baudrate}"

setenv bootargs
for BOOT_SLOT in "${BOOT_ORDER}"; do
  if test "x${bootargs}" != "x"; then
    # skip remaining slots
  elif test "x${BOOT_SLOT}" = "xA"; then
    if test 0x${BOOT_A_LEFT} -gt 0; then
      echo "Found valid slot A, ${BOOT_A_LEFT} attempts remaining"
      setexpr BOOT_A_LEFT ${BOOT_A_LEFT} - 1
      setenv loadimage "ext4load mmc 1:1 ${loadaddr} /boot/${image}"
      setenv loadfdt "ext4load mmc 1:1 ${fdt_addr} /boot/${fdt_file}"
      setenv bootargs "${default_bootargs} root=/dev/mmcblk1p1 rootwait rw rauc.slot=A"
    fi
  elif test "x${BOOT_SLOT}" = "xB"; then
    if test 0x${BOOT_B_LEFT} -gt 0; then
      echo "Found valid slot B, ${BOOT_B_LEFT} attempts remaining"
      setexpr BOOT_B_LEFT ${BOOT_B_LEFT} - 1
      setenv loadimage "ext4load mmc 1:2 ${loadaddr} /boot/${image}"
      setenv loadfdt "ext4load mmc 1:2 ${fdt_addr} /boot/${fdt_file}"
      setenv bootargs "${default_bootargs} root=/dev/mmcblk1p2 rootwait rw rauc.slot=B"
    fi
  fi
done


if test -n "${bootargs}"; then
  saveenv
else
  echo "No valid slot found, resetting tries to 3"
  setenv BOOT_A_LEFT 3
  setenv BOOT_B_LEFT 3
  saveenv
  reset
fi

echo "Loading dtb"
${loadfdt}
echo "Loading kernel"
${loadimage}
echo " Starting kernel"
bootz ${loadaddr} - ${fdt_addr}