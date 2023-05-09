# -*- mode: makefile-gmake -*-

LIBYACHTROCK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

define newline


endef

$(eval $(subst ^,$(newline),$(shell YACHTROCK_UNIXY=$(YACHTROCK_UNIXY) YACHTROCK_POSIXY=$(YACHTROCK_POSIXY) \
	YACHTROCK_DLOPEN=$(YACHTROCK_DLOPEN) YACHTROCK_MULTIPROCESS=$(YACHTROCK_MULTIPROCESS) \
	$(LIBYACHTROCK_DIR)detect_base_config.sh | tr '\n' '^' )))

$(LIBYACHTROCK_DIR)public_headers/yachtrock/config.h: $(LIBYACHTROCK_DIR)write_config_h.sh
	YACHTROCK_UNIXY=$(YACHTROCK_UNIXY) YACHTROCK_POSIXY=$(YACHTROCK_POSIXY) \
		YACHTROCK_DLOPEN=$(YACHTROCK_DLOPEN) YACHTROCK_MULTIPROCESS=$(YACHTROCK_MULTIPROCESS) ./$< > $@

YACHTROCK_GENERATED_HEADERS := $(LIBYACHTROCK_DIR)public_headers/yachtrock/config.h
GENERATED_HEADERS += $(YACHTROCK_GENERATED_HEADERS)

LIBYACHTROCK_DYLIBNAME := $(shell $(YAP_GENERATE_DYLIBNAME) yachtrock)

YR_RUNTESTS := yr_runtests

all: $(LIBYACHTROCK_DYLIBNAME)

ifeq ($(YACHTROCK_DLOPEN),1)
all: $(YR_RUNTESTS)
dstroot_install: dstroot_install_yr_runtests
endif

clean: clean_libyachtrock

LIBYACHTROCK_GENERATED_SRC := 
LIBYACHTROCK_GENERATED_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_GENERATED_SRC))
LIBYACHTROCK_STATIC_SRC := yachtrock.c runtime.c testcase.c results.c yrutil.c selector.c script_helpers.c version.c
ifeq ($(YACHTROCK_MULTIPROCESS),1)
LIBYACHTROCK_STATIC_SRC += multiprocess.c multiprocess_inferior.c multiprocess_superior.c
endif
LIBYACHTROCK_STATIC_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_STATIC_SRC))
YR_RUNTESTS_SRC := yr_runtests.c
YR_RUNTESTS_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(YR_RUNTESTS_SRC))
LIBYACHTROCK_SRC := $(LIBYACHTROCK_STATIC_SRC) $(LIBYACHTROCK_GENERATED_SRC)
LIBYACHTROCK_HEADER_SOURCES := $(wildcard $(LIBYACHTROCK_DIR)public_headers/yachtrock/*.h)
LIBYACHTROCK_HEADER_INSTALLED_FILES := $(subst $(LIBYACHTROCK_DIR)public_headers,$(DSTROOT_AND_PREFIX)/include,$(LIBYACHTROCK_HEADER_SOURCES))

CSRC += $(LIBYACHTROCK_SRC) $(YR_RUNTESTS_SRC)
LIBYACHTROCK_OBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_SRC)))
LIBYACHTROCK_LINKS = -lpthread
LIBYACHTROCK_YAP_LINK_OPTIONS = --stdthreads --sockets --dl

YR_RUNTESTS_OBJ := $(patsubst %.c,%.o,$(filter %.c,$(YR_RUNTESTS_SRC)))

$(YR_RUNTESTS_OBJ): CFLAGS += -D_POSIX_C_SOURCE=200809L

include $(LIBYACHTROCK_DIR)module_tests.mk

$(LIBYACHTROCK_STATIC_SRC): $(LIBYACHTROCK_GENERATED_SRC)

$(LIBYACHTROCK_OBJ): CFLAGS += -fPIC -D_POSIX_C_SOURCE=200809L -Wmissing-prototypes

$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_OBJ)
	$(YAP_LINK) --driver $(CC) --dynamic_install_name $(PREFIX)/lib/$@ $(LIBYACHTROCK_YAP_LINK_OPTIONS) -- $(LDFLAGS) $(LIBYACHTROCK_OBJ) $(LIBYACHTROCK_LINKS) -o $@

$(YR_RUNTESTS): $(YR_RUNTESTS_OBJ) $(LIBYACHTROCK_DYLIBNAME)
	$(YAP_LINK) --driver $(CC) --links_self_using_prefix "$(PREFIX)" --dl -- $(LDFLAGS) $^ -o $@

clean_libyachtrock:
	rm -f $(LIBYACHTROCK_DYLIBNAME)
	rm -f $(LIBYACHTROCK_OBJ)
	rm -f $(LIBYACHTROCK_GENERATED_SRC)
	rm -f $(YACHTROCK_GENERATED_HEADERS)
	$(LIBYACHTROCK_DIR)detect_base_config.sh --clean

dstroot_install: dstroot_install_libyachtrock_dylib dstroot_install_libyachtrock_headers

dstroot_install_yr_runtests: $(DSTROOT_AND_PREFIX)/bin/$(YR_RUNTESTS)

$(DSTROOT_AND_PREFIX)/bin/$(YR_RUNTESTS): $(YR_RUNTESTS) $(DSTROOT_AND_PREFIX)/bin
	$(YAP_INSTALL) -m 0755 -vc $< $(DSTROOT_AND_PREFIX)/bin

dstroot_install_libyachtrock_dylib: $(DSTROOT_AND_PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)

$(DSTROOT_AND_PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_DYLIBNAME) $(DSTROOT_AND_PREFIX)/lib
	$(YAP_INSTALL) -vc $< $(DSTROOT_AND_PREFIX)/lib

dstroot_install_libyachtrock_headers: libyachtrock_headers_installation_dir libyachtrock_installed_headers

libyachtrock_headers_installation_dir: $(DSTROOT_AND_PREFIX)/include/yachtrock

$(DSTROOT_AND_PREFIX)/include/yachtrock:
	$(YAP_INSTALL) -dv $(DSTROOT_AND_PREFIX)/include/yachtrock

libyachtrock_installed_headers: $(LIBYACHTROCK_HEADER_INSTALLED_FILES)

$(DSTROOT_AND_PREFIX)/include/yachtrock/%.h: $(LIBYACHTROCK_DIR)public_headers/yachtrock/%.h
	$(YAP_INSTALL) -m 0644 -vc $< $(DSTROOT_AND_PREFIX)/include/yachtrock

libyachtrock_install_dev_dylib_links: $(PREFIX)/lib
	ln -s `pwd`/$(LIBYACHTROCK $(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)
