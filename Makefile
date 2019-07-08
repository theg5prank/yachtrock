# -*- mode: makefile-gmake -*-

INSTALLER:=./install_wrapper.sh

ifeq ($(shell uname),SunOS)
	CC=gcc
endif

IS_GCC := $(shell ($(CC) --version | grep 'Copyright.*Free Software Foundation') > /dev/null && echo "YES")

ifeq ($(IS_GCC),YES)
CFLAGS += -std=c11
endif

include configuration.mk

CFLAGS += -I. -Wall
CXXFLAGS += -I. -Wall

CSRC :=
CPPSRC :=

MODULES := .
CFLAGS += $(patsubst %,-I%,$(MODULES))
CXXFLAGS += $(patsubst %,-I%,$(MODULES))
CFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))
CXXFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))

TESTS :=

PRODUCTS :=

MODULE_CLEANS :=

GENERATED_HEADERS := 

include $(patsubst %,%/module.mk,$(MODULES))

ALLOBJ := $(patsubst %.c,%.o,$(filter %.c,$(CSRC)))
ALLOBJ += $(patsubst %.cc,%.o,$(filter %.cc,$(CPPSRC)))

include $(ALLOBJ:.o=.d)

OBJ := $(filter-out $(TESTOBJ),$(ALLOBJ))

$(ALLOBJ:.o=.d): $(GENERATED_HEADERS)

%.d: %.cc
	./depend.sh `dirname $*.cc` $(CXXFLAGS) $*.cc > $@
%.d: %.c
	./depend.sh `dirname $*.c` $(CFLAGS) $*.c > $@

test: $(TESTS)

clean: $(MODULE_CLEANS)
	rm -f $(ALLOBJ)
	rm -f $(TESTS)
	rm -f $(ALLOBJ:.o=.d)
	rm -f $(PRODUCTS)

.PHONY:debugmk
debugmk:
	echo $(ALLOBJ)

all: $(PRODUCTS)

$(PREFIX)/lib:
	$(INSTALLER) -d $(PREFIX)/lib

$(PREFIX)/include:
	$(INSTALLER) -d $(PREFIX)/include

install: all

.DEFAULT_GOAL := all
