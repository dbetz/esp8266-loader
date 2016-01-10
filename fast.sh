FILE=LargeSpinCode.binary
SIZE=32468
#FILE=blink_fast.binary
#SIZE=1024
#FILE=toggle.binary
#SIZE=6800
#MODULE=thing2.local
MODULE=10.0.1.47
curl -v -X POST $MODULE/load-begin?size=$SIZE
curl -v -X POST -H "Expect:" --data-binary @$FILE $MODULE/load-data 
curl -v -X POST $MODULE/load-end?command=run
