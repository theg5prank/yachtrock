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

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
LIBYACHTROCK_DYLIBNAME := libyachtrock.dylib
else
LIBYACHTROCK_DYLIBNAME := libyachtrock.so
endif

LIBYACHTROCK_ARNAME := libyachtrock.a

YR_RUNTESTS := yr_runtests

PRODUCTS += $(LIBYACHTROCK_DYLIBNAME) $(LIBYACHTROCK_ARNAME)

ifeq ($(YACHTROCK_MULTIPROCESS)$(YACHTROCK_DLOPEN),11)
PRODUCTS += $(YR_RUNTESTS)
endif

MODULE_CLEANS += clean_libyachtrock

LIBYACHTROCK_GENERATED_SRC := 
LIBYACHTROCK_GENERATED_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_GENERATED_SRC))
LIBYACHTROCK_STATIC_SRC := yachtrock.c runtime.c testcase.c results.c yrutil.c
ifeq ($(YACHTROCK_MULTIPROCESS),1)
LIBYACHTROCK_STATIC_SRC += multiprocess.c multiprocess_inferior.c multiprocess_superior.c
endif
LIBYACHTROCK_STATIC_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_STATIC_SRC))
YR_RUNTESTS_SRC := yr_runtests.c
YR_RUNTESTS_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(YR_RUNTESTS_SRC))
LIBYACHTROCK_SRC := $(LIBYACHTROCK_STATIC_SRC) $(LIBYACHTROCK_GENERATED_SRC)
LIBYACHTROCK_HEADER_SOURCES := $(wildcard $(LIBYACHTROCK_DIR)public_headers/yachtrock/*.h)
LIBYACHTROCK_HEADER_INSTALLED_FILES := $(subst $(LIBYACHTROCK_DIR)public_headers,$(PREFIX)/include,$(LIBYACHTROCK_HEADER_SOURCES))

CSRC += $(LIBYACHTROCK_SRC) $(YR_RUNTESTS_SRC)
LIBYACHTROCK_OBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_SRC)))
LIBYACHTROCK_LINKS = -ldl -lpthread

YR_RUNTESTS_OBJ := $(patsubst %.c,%.o,$(filter %.c,$(YR_RUNTESTS_SRC)))

include $(LIBYACHTROCK_DIR)module_tests.mk

$(LIBYACHTROCK_STATIC_SRC): $(LIBYACHTROCK_GENERATED_SRC)

$(LIBYACHTROCK_OBJ): CFLAGS += -fPIC -D_POSIX_C_SOURCE=200809L -Wmissing-prototypes

$(LIBYACHTROCK_ARNAME): $(LIBYACHTROCK_OBJ)
	ar rc $@ $(LIBYACHTROCK_OBJ)
	ranlib $@

ifeq ($(UNAME_S),Darwin)
$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_OBJ)
	cc -dynamiclib $(LIBYACHTROCK_OBJ) -install_name $(PREFIX)/lib/$@ $(LIBYACHTROCK_LINKS) -o $@
else
$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_OBJ)
	cc -shared $(LIBYACHTROCK_OBJ) $(LIBYACHTROCK_LINKS) -Wl,-rpath,$(PREFIX)/lib/$@ -o $@
endif

$(YR_RUNTESTS): $(YR_RUNTESTS_OBJ) $(LIBYACHTROCK_DYLIBNAME)
	$(CC) $^ -o $@

clean_libyachtrock:
	rm -f $(LIBYACHTROCK_ARNAME)
	rm -f $(LIBYACHTROCK_DYLIBNAME)
	rm -f $(LIBYACHTROCK_OBJ)
	rm -f $(LIBYACHTROCK_GENERATED_SRC)
	rm -f $(YACHTROCK_GENERATED_HEADERS)
	$(LIBYACHTROCK_DIR)detect_base_config.sh --clean

install: install_libyachtrock_dylib install_libyachtrock_headers

ifneq (($filter $(YR_RUNTESTS),$(PRODUCTS)),)
install: install_yr_runtests
endif

install_yr_runtests: $(PREFIX)/bin/$(YR_RUNTESTS)

$(PREFIX)/bin/$(YR_RUNTESTS): $(YR_RUNTESTS) $(PREFIX)/bin
	install -vc $< $(PREFIX)/bin

install_libyachtrock_dylib: $(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)

$(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_DYLIBNAME) $(PREFIX)/lib
	install -vc $< $(PREFIX)/lib

install_libyachtrock_headers: libyachtrock_headers_installation_dir libyachtrock_installed_headers

libyachtrock_headers_installation_dir: $(PREFIX)/include/yachtrock

$(PREFIX)/include/yachtrock:
	install -dv $(PREFIX)/include/yachtrock

libyachtrock_installed_headers: $(LIBYACHTROCK_HEADER_INSTALLED_FILES)

$(PREFIX)/include/yachtrock/%.h: $(LIBYACHTROCK_DIR)public_headers/yachtrock/%.h
	install -m 0644 -vc $< $(PREFIX)/include/yachtrock

libyachtrock_install_dev_dylib_links: $(PREFIX)/lib
	ln -s `pwd`/$(LIBYACHTROCK $(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)
