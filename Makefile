
MAKE = /usr/bin/make

all:
	$(MAKE) -C comp/
	$(MAKE) -C memory/
	$(MAKE) -C svm/

clean:
	$(MAKE) clean -C comp/
	$(MAKE) clean -C memory/
	$(MAKE) clean -C svm/
	
