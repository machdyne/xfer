build:
	gcc -o xfer xfer.c

clean:
	rm -f xfer

install:
	cp xfer /usr/local/bin

.PHONY: build install
