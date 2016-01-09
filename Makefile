all:	IP_Loader.h binaries

IP_Loader.h:	IP_Loader.spin tools/split
	openspin IP_loader.spin
	./tools/split IP_Loader.binary IP_Loader.h

tools/split:
	$(MAKE) -C tools

binaries:
	openspin -DMINI -o blink_fast.binary blink.spin
	openspin -DMINI -DSLOW -o blink_slow.binary blink.spin
	propeller-elf-gcc -mlmm -Os -o toggle.elf toggle.c
	propeller-load -s toggle.elf

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
	$(MAKE) -C tools clean
	rm -f *.binary *.elf IP_Loader.h
