CFLAGS=-Wall

all:	split bin2c

split:	split.c
	cc $(CFLAGS) -o split $<

bin2c:	bin2c.c
	cc $(CFLAGS) -o bin2c $<

clean:
	rm -f split bin2c
