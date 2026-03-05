all: build

build:
	$(MAKE) -C lib
	$(MAKE) -C pp
	$(MAKE) -C cc

test:
	$(MAKE) -C lib test
	$(MAKE) -C pp test
	$(MAKE) -C cc test

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C pp clean
	$(MAKE) -C cc clean
