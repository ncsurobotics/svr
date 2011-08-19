
# These are all of the default or common configuration options

# Build options
CC ?= gcc
CFLAGS = -std=c99 -Wall -Werror -pedantic -Wmissing-prototypes -g

# Include /usr/local in include and library search paths
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib

# Install prefix
#PREFIX ?= /usr/local

# Component names
LIB_NAME = libsvr.so
SERVER_NAME = svr

# Extra LDFLAGS for some systems
EXTRA_LDFLAGS = 

# Library version
MAJOR = 0
MINOR = 1
REV = 0

# Attempt to automatically determine the host type
ifndef CONFIG
  HOSTTYPE = $(strip $(shell uname -s))

  ifeq ($(HOSTTYPE), Linux)
    CONFIG = mk/config.linux.mk
  else
    # Fall back to a generic set of options
    CONFIG = mk/config.generic.mk
  endif
endif
