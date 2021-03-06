PORT=/dev/cu.usbserial-A700fKXl

all:	esp8266-firmware/IP_Loader.h binaries

esp8266-firmware/IP_Loader.h:	esp8266-firmware/IP_Loader.spin tools/split
	openspin esp8266-firmware/IP_Loader.spin
	./tools/split esp8266-firmware/IP_Loader.binary esp8266-firmware/IP_Loader.h

tools/split:
	$(MAKE) -C tools

binaries:
	openspin -DMINI -o blink_fast.binary tests/blink.spin
	openspin -DMINI -DSLOW -o blink_slow.binary tests/blink.spin
	openspin -o LargeSpinCode.binary tests/LargeSpinCode.spin
	propeller-elf-gcc -mlmm -Os -o toggle.elf tests/toggle.c
	propeller-load -s toggle.elf

run-fast:	binaries
	curl -X POST --data-binary @blink_fast.binary thing2.local/run

run-slow:	binaries
	curl -X POST --data-binary @blink_slow.binary thing2.local/run

fast:
	curl -X POST --data-binary @blink_slow.binary thing2.local/run
	curl -X POST --data-binary @blink_slow.binary thing2.local/run
	curl -X POST --data-binary @blink_slow.binary thing2.local/run

install:
	esptool -vv -cd ck -cb 115200 -cp $(PORT) -ca 0x00000 -cf esp8266-loader.ino.generic.bin

zip:
	zip -r ../esp8266-loader-$(shell date "+%Y-%m-%d").zip *

clean:
	$(MAKE) -C tools clean
	rm -f *.binary *.elf IP_Loader.h
