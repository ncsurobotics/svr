
include mk/config.base.mk
include $(CONFIG)

# Ensure that PREFIX is saved as an absolute path
export PREFIX := $(abspath $(PREFIX))

all: lib server python util

lib:
	cd lib && $(MAKE) $(LIB_FILE)

server: lib
	cd server && $(MAKE) $(SERVER_NAME)

python: lib
	cd python && $(PYTHON) setup.py build

util: lib
	cd util && $(MAKE)

install: lib-install server-install python-install util-install

uninstall: lib-uninstall server-uninstall util-uninstall

lib-install:
	cd lib && $(MAKE) install

lib-uninstall:
	cd lib && $(MAKE) uninstall

server-install:
	cd server && $(MAKE) install

server-uninstall:
	cd server && $(MAKE) uninstall

util-install:
	cd util && $(MAKE) install

util-uninstall:
	cd util && $(MAKE) uninstall

python-install:
	cd python && $(PYTHON) setup.py install

tests:
	cd test && $(MAKE)

clean:
	cd lib && $(MAKE) $@
	cd server && $(MAKE) $@
	cd util && $(MAKE) $@
	-rm -rf doc/html/ 2> /dev/null
	-rm -rf doc/server/html/ 2> /dev/null

doc:
	doxygen doc/Doxyfile

doc-hub:
	doxygen doc/server/Doxyfile

.PHONY: all lib server python util install uninstall python-install lib-install	\
    lib-uninstall server-install server-uninstall util-install util-uninstall \
    tests clean doc doc-hub
