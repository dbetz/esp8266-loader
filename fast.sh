FILE=$1
SIZE=`ls -l $FILE | cut -c27-32`
SIZE=${SIZE// /}
echo Loading $FILE, ${SIZE} bytes
curl -v -X POST $MODULE/load-begin?size=$SIZE\&reset-pin=2
curl -v -X POST -H "Expect:" --data-binary @$FILE $MODULE/load-data 
curl -v -X POST $MODULE/load-end?command=run
telnet $MODULE
