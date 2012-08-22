
# These are all of the default or common configuration options

# Build options
CC ?= gcc
CFLAGS = -std=gnu99 -Wall -Werror -Wno-unused-function -pedantic -Wmissing-prototypes -g

# Enable dummy allocator
#CFLAGS += -DSVR_DUMMY_ALLOC

# Python executable
PYTHON ?= python

# Include /usr/local in include and library search paths
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib

# OpenCV flags
CV_CFLAGS ?= $(strip $(shell pkg-config --cflags opencv))
CV_LDFLAGS ?= $(strip $(shell pkg-config --libs opencv))

# Install prefix
#PREFIX ?= /usr/local

# Library version
MAJOR = 0
MINOR = 1
REV = 0

# Component names
LIB_NAME = svr
SERVER_NAME = svrd

# Extra LDFLAGS for some systems
EXTRA_LDFLAGS =

# Library output file
LIB_FILE = lib$(LIB_NAME).so.$(MAJOR)
LIB_FILE_BASE = lib$(LIB_NAME).so

# Arguments to the compiler for building the shared library
LIB_LDFLAGS = --shared -Wl,-soname,$(LIB_FILE)

# Attempt to automatically determine the host type
ifndef CONFIG
  HOSTTYPE = $(strip $(shell uname -s))

  ifeq ($(HOSTTYPE), Linux)
    CONFIG = mk/config.linux.mk
  else ifeq ($(HOSTTYPE), Darwin)
    CONFIG = mk/config.darwin.mk
  else
    # Fall back to a generic set of options
    CONFIG = mk/config.generic.mk
  endif
endif
