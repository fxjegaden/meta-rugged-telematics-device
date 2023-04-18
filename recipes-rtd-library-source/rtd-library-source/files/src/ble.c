#include "wifi_ble.h"

int ble_init( )
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	char hcattach_conf[ ] = "hciattach /dev/ttymxc3 bcm43xx 3000000 noflow -t 20";
	char hci_up[ ] = "hciconfig hci0 up";

	ret = gpio_export(133, OUTPUT);
	CHK_ERR (ret, stderr, "Error in ble_init() - BLUETOOTH MODULE export");
	sleep(1);
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_INIT;
		goto exit;
	}

	ret = set_gpio_value(133, ON);
	CHK_ERR (ret, stderr, "Error: ble_init() - set BLUETOOTH MODULE SWITCH Highh");
	sleep(1);
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_INIT;
		goto exit;
	}

	ret = system( hcattach_conf );
	CHK_ERR (ret, stderr, "Error: ble_init() - hci interface configuration");
	sleep(1);
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_INIT;
		goto exit;
	}

	ret = system( hci_up );
	CHK_ERR (ret, stderr, "Error: ble_init() - hci interface up");
	sleep(1);
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_INIT;
		goto exit;
	}

exit:
	return error;
}

int ble_deinit( )
{
	int ret = OBD2_LIB_SUCCESS;
	int error = OBD2_LIB_SUCCESS;
	char hci_down[ ] = "hciconfig hci0 down";

	ret = system( hci_down );
	CHK_ERR (ret, stderr, "Error: ble_init() - hci interface down");
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_DEINIT;
		goto exit;
	}

	ret = set_gpio_value(133, OFF);
	CHK_ERR (ret, stderr, "Error: ble_init() - set BLUETOOTH MODULE SWITCH Low");
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_DEINIT;
		goto exit;
	}

	ret = system( "killall -9 hciattach" );
	CHK_ERR (ret, stderr, "Error: ble_init() killall hciattach");
	if( ret < OBD2_LIB_SUCCESS )
	{
		error = E_OBD2_LIB_BLE_DEINIT;
		goto exit;
	}

exit:
	return error;
}
