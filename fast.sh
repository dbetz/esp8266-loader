FILE=blink_fast.binary
SIZE=56
#FILE=toggle.binary
#SIZE=6800
#MODULE=thing2.local
MODULE=10.0.1.47
curl -v -X POST $MODULE/load-begin?size=$SIZE\&final-baud-rate=115200
curl -v -X POST --data-binary @$FILE $MODULE/load-data 
curl -v -X POST $MODULE/load-end?command=run
