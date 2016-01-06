binaries:
	openspin -DMINI -o blink_fast.binary blink.spin
	openspin -DMINI -DSLOW -o blink_slow.binary blink.spin

run-fast:	binaries
	curl -X POST --data-binary @blink_fast.binary thing2.local/run

run-slow:	binaries
	curl -X POST --data-binary @blink_slow.binary thing2.local/run

fast:
	curl -X POST --data-binary @blink_slow.binary thing2.local/run
	curl -X POST --data-binary @blink_slow.binary thing2.local/run
	curl -X POST --data-binary @blink_slow.binary thing2.local/run

zip:
	zip -r ../esp8266-loader-$(shell date "+%Y-%m-%d").zip *

clean:
	rm -f *.binary
