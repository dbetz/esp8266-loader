FILE=$1
SIZE=`ls -l $FILE | cut -c28-33`
SIZE=${SIZE// /}
echo Loading $FILE, ${SIZE} bytes
MODULE=10.0.1.47
curl -v -X POST $MODULE/load-begin?size=$SIZE
curl -v -X POST -H "Expect:" --data-binary @$FILE $MODULE/load-data 
curl -v -X POST $MODULE/load-end?command=run
telnet $MODULE
