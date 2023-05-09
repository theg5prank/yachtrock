# -*- mode: makefile-gmake -*-

# define toplevel actions first.

all:

clean:

test:

# set helper variables.

YAP_INSTALL := ./yap/install.sh
YAP_LINK := ./yap/link.sh
YAP_GENERATE_DYLIBNAME := ./yap/generate_dylibname.sh
YAP_UNAME_S := $(shell uname -s)

ifdef PLATFORM
YAP_PLATFORM_SUFFIX := -$(PLATFORM)
else
YAP_PLATFORM_SUFFIX :=
endif

ifeq ($(shell uname),SunOS)
	CC=gcc
endif

YAP_IS_GCC := $(shell ($(CC) --version | grep 'Copyright.*Free Software Foundation') > /dev/null && echo "YES")

# set default flags.

CFLAGS += -std=c11 -Wall
CXXFLAGS += -std=c++17 -Wall
CFLAGS += -I.
CXXFLAGS += -I.
CFLAGS += $(patsubst %,-I%,$(MODULES))
CXXFLAGS += $(patsubst %,-I%,$(MODULES))
CFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))
CXXFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))

# define recipes for depend files.

%$(YAP_PLATFORM_SUFFIX).d: %.c
	./yap/depend.sh `dirname $*.c` $(CFLAGS) $*.c > $@

%$(YAP_PLATFORM_SUFFIX).d: %.cc
	./yap/depend.sh `dirname $*.cc` $(CXXFLAGS) $*.cc > $@

# define filelist variables for modules to add to.

BUILD_ALLOBJ = $(patsubst %.c,%.o,$(filter %.c,$(CSRC)))
BUILD_ALLOBJ += $(patsubst %.cc,%.o,$(filter %.cc,$(CPPSRC)))
ifdef PLATFORM
ALLOBJ = $(BUILD_ALLOBJ:.o=-$(PLATFORM).o)
else
ALLOBJ = $(BUILD_ALLOBJ)
endif
ALLDEPEND = $(ALLOBJ:.o=.d)

# include configuration.
-include configuration.mk

ifndef PREFIX
	$(error PREFIX must be set via configuration.mk, even if to /)
endif

ifdef DSTROOT
DSTROOT_AND_PREFIX := $(DSTROOT)$(PREFIX)
else
DSTROOT_AND_PREFIX := $(PREFIX)
endif

# include the project's modules, and then the defined depend files.

include $(MODULES:=/module.mk)

# hack: we need to add the dependency on generated headers now. If attempted before including modules,
# the variables aren't defined yet so the dependencies don't take.
$(ALLDEPEND): $(GENERATED_HEADERS)

include $(ALLDEPEND)

# help out with some basic action prerequisites.

clean_allobj:
	rm -f $(ALLOBJ)

clean_alldepend:
	rm -f $(ALLDEPEND)

clean: clean_allobj clean_alldepend

$(PREFIX)/bin:
	$(YAP_INSTALL) -d $@

$(PREFIX)/sbin:
	$(YAP_INSTALL) -d $@

$(PREFIX)/lib:
	$(YAP_INSTALL) -d $(PREFIX)/lib

$(PREFIX)/include:
	$(YAP_INSTALL) -d $(PREFIX)/include

ifdef DSTROOT
$(DSTROOT_AND_PREFIX)/bin:
	$(YAP_INSTALL) -d $@

$(DSTROOT_AND_PREFIX)/sbin:
	$(YAP_INSTALL) -d $@

$(DSTROOT_AND_PREFIX)/lib:
	$(YAP_INSTALL) -d $(DSTROOT_AND_PREFIX)/lib

$(DSTROOT_AND_PREFIX)/include:
	$(YAP_INSTALL) -d $(DSTROOT_AND_PREFIX)/include
endif

install: dstroot_install

.DEFAULT_GOAL := all
