
include mk/config.base.mk
include $(CONFIG)

# Ensure that PREFIX is saved as an absolute path
export PREFIX := $(abspath $(PREFIX))

all: lib server python

lib:
	cd lib && $(MAKE) $(LIB_NAME)

server: lib
	cd server && $(MAKE) $(SERVER_NAME)

python: lib
	cd python && $(PYTHON) setup.py build

install: lib-install server-install python-install

uninstall: lib-uninstall server-uninstall

lib-install:
	cd lib && $(MAKE) install

lib-uninstall:
	cd lib && $(MAKE) uninstall

server-install:
	cd server && $(MAKE) install

sever-uninstall:
	cd server && $(MAKE) uninstall

python-install:
	cd python && $(PYTHON) setup.py install

tests:
	cd test && $(MAKE)

clean:
	cd lib && $(MAKE) $@
	cd server && $(MAKE) $@
	-rm -rf doc/html/ 2> /dev/null
	-rm -rf doc/server/html/ 2> /dev/null

doc:
	doxygen doc/Doxyfile

doc-hub:
	doxygen doc/server/Doxyfile

.PHONY: all lib server python install uninstall python-install lib-install \
	lib-uninstall server-install server-uninstall tests clean doc doc-hub
