HOSTIP=`ifconfig ppp0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'`
echo "$HOSTIP"
while [ 1 ];
do
	if ping -q -c 1 -W 1 google.com >/dev/null; 
	then
	  echo "4g Connection is up"
	  break
	else
	  echo "4g Connection is down"
	  pppd call gprs_4g
	fi
done
ping google.com &
