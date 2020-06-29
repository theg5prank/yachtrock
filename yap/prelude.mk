# -*- mode: makefile-gmake -*-

# define toplevel actions first.

all:

clean:

test:

# set helper variables.

YAP_INSTALL := ./yap/install.sh
YAP_LINK := ./yap/link.sh
YAP_UNAME_S := $(shell uname -s)

ifeq ($(shell uname),SunOS)
	CC=gcc
endif

YAP_IS_GCC := $(shell ($(CC) --version | grep 'Copyright.*Free Software Foundation') > /dev/null && echo "YES")

# set default flags.

CXXFLAGS += -std=c++17 -Wall
CFLAGS += -std=c11 -Wall
CFLAGS += $(patsubst %,-I%,$(MODULES))
CXXFLAGS += $(patsubst %,-I%,$(MODULES))
CFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))
CXXFLAGS += $(patsubst %,-I%/public_headers,$(MODULES))

# define recipes for depend files.

%.d: %.c
	./yap/depend.sh `dirname $*.c` $(CFLAGS) $*.c > $@

%.d: %.cc
	./yap/depend.sh `dirname $*.cc` $(CXXFLAGS) $*.cc > $@

# define filelist variables for modules to add to.

ALLOBJ = $(patsubst %.c,%.o,$(filter %.c,$(CSRC)))
ALLOBJ += $(patsubst %.cc,%.o,$(filter %.cc,$(CPPSRC)))
ALLDEPEND = $(ALLOBJ:.o=.d)

# include configuration.
-include configuration.mk

ifndef PREFIX
	$(error PREFIX must be set via configuration.mk, even if to /)
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

$(PREFIX)/lib:
	$(YAP_INSTALL) -d $(PREFIX)/lib

$(PREFIX)/include:
	$(YAP_INSTALL) -d $(PREFIX)/include

.DEFAULT_GOAL := all
