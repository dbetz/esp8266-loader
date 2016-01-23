FILE=$1
SIZE=`ls -l $FILE | cut -c28-33`
SIZE=${SIZE// /}
echo Loading $FILE, ${SIZE} bytes
MODULE=10.0.1.18
curl -v -X POST $MODULE/stamp
telnet $MODULE
