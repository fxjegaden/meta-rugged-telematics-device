#Initializing two variables
while [ 1 ]
do
        a=$(pidof rfcomm)
        echo "PID $a"
        b=0
        if [ -z $a ]
        then
                rfcomm --raw listen /dev/rfcomm0 2 &
        fi
done


