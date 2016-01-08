curl -v -X POST thing2.local/load-begin?size=56\&final-baud-rate=115200
curl -v -X POST --data-binary @blink_fast.binary thing2.local/load-data 
curl -v -X POST thing2.local/load-end?command=run
