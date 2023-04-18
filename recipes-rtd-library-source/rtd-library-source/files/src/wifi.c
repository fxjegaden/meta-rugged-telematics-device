#include "thread.h"
#include "wifi_ble.h"
#include "lib_common.h"

/*
 * API to be called immediately after wakeup
 * Loads Wifi driver
 * Make wlan0 interface up
 */
int wifi_init( int mode )
{
	int ret = OBD2_LIB_FAILURE;
	int error = OBD2_LIB_FAILURE;
	if (mode == 1 || mode == 0)
	{
		wifi_mode = mode;
	}
	else
		return E_OBD2_LIB_INVALID_ARG;


	ret = CheckLink("wlan0");
	if (ret == OBD2_LIB_SUCCESS){
		ret = E_OBD2_LIB_WIFI_IS_ACTIVE;
		return ret;
	}

	ret = system("lsmod | grep brcmfmac > /dev/null");
	if (ret != OBD2_LIB_SUCCESS)
	{
		ret = system("modprobe brcmfmac");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_MOD_INIT;
			goto exit;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}
	sleep( 3 );

	if( wifi_mode == WIFI_HOST_MODE )
	{
		ret = set_wifi_hostapd_mode();
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = ret;
			goto exit;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}
	else if( wifi_mode == WIFI_STA_MODE )
	{
		ret = set_wifi_station_mode();
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = ret;
			goto exit;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}

exit:
	return error;
}

int wifi_deinit()
{
	int ret = OBD2_LIB_FAILURE;
	int error = OBD2_LIB_FAILURE;
	int buf = 0;
	FILE *fp = NULL;

	/* Turn off WiFi Station Mode */
	if((fp = popen("pidof wpa_supplicant", "r")) != NULL)
	{
		ret = fscanf(fp, "%d", &buf);
		if(ret > 0)
		{
			printf("wpa_supplicant is enabled\n");
			ret = system("killall -9 wpa_supplicant");
			ret = system("killall -9 udhcpc");
		}
	}
	else
	{
		pclose(fp);
	}

	/* Turn off WiFi Host Mode */
	if((fp = popen("pidof hostapd", "r")) != NULL)
	{
		ret = fscanf(fp, "%d", &buf);
		if(ret > 0)
		{
			printf("hostapd is enabled\n");
			ret = system("killall -9 hostapd");
			ret = system("killall -9 udhcpd");
		}
	}
	else
	{
		pclose(fp);
	}

	ret = CheckLink("wlan0");
	if(ret == OBD2_LIB_SUCCESS)
	{
		ret = system("ifconfig wlan0 down");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_UP;
			goto exit;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}

	/* Remove wifi Driver */
	ret = system("lsmod | grep brcmfmac > /dev/null");
	if(ret == OBD2_LIB_SUCCESS)
	{
		ret = system("modprobe -r brcmfmac");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_MOD_DEINIT;
			goto exit;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}

	ret = CheckLink("wlan0");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = OBD2_LIB_SUCCESS;
	}

exit:
	return error;
}

int set_wifi_hostapd_mode( )
{
	int ret = OBD2_LIB_FAILURE;
	int error = OBD2_LIB_FAILURE;

	ret = system("killall -9 hostapd");
	sleep( 3 );

	/* interface up */
	ret = system("hostapd /etc/hostapd.conf -B");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_HOST_INIT;
		goto exit;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}
	sleep( 3 );

	ret = system("ifconfig wlan0 192.168.43.1");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_WIFI_IP_SET;
		goto exit;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}
	sleep( 3 );

	ret = system("udhcpd");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_WIFI_UDHCPD;
		goto exit;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}
	sleep( 3 );

	ret = CheckLink("ppp0");
	if (ret == OBD2_LIB_SUCCESS)
	{
		/* ip forwarding */
		ret = system("iptables -F");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("iptables -t nat -F");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("echo 1 > /proc/sys/net/ipv4/ip_forward");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("iptables -A FORWARD -i wlan0 -o ppp0 -j ACCEPT");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}

		ret = system("iptables -t nat -S");
		if(ret != OBD2_LIB_SUCCESS)
		{
			error = E_OBD2_LIB_WIFI_IPTABLES;
		}
		else
		{
			error = OBD2_LIB_SUCCESS;
		}
	}

exit:
	return error;
}

int set_wifi_station_mode( )
{
	int ret = OBD2_LIB_FAILURE;
	int error = OBD2_LIB_FAILURE;

	ret = system("killall -9 wpa_supplicant");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_STA_DEINIT;
	}
	sleep( 3 );

	ret = system("wpa_supplicant -d -Dnl80211 -c /etc/wpa_supplicant.conf -i wlan0 &");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_STA_INIT;
		goto exit;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}

	sleep( 3 );

	ret = system("udhcpc -i wlan0");
	if(ret != OBD2_LIB_SUCCESS)
	{
		error = E_OBD2_LIB_STA_UDHCPC;
		goto exit;
	}
	else
	{
		error = OBD2_LIB_SUCCESS;
	}

exit:
	return error;
}
