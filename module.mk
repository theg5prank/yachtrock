LIBYACHTROCK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
LIBYACHTROCK_DYLIBNAME := libyachtrock.dylib
else
LIBYACHTROCK_DYLIBNAME := libyachtrock.so
endif

LIBYACHTROCK_ARNAME := libyachtrock.a

YR_RUNTESTS := yr_runtests

PRODUCTS += $(LIBYACHTROCK_DYLIBNAME) $(LIBYACHTROCK_ARNAME) $(YR_RUNTESTS)

MODULE_CLEANS += clean_libyachtrock

LIBYACHTROCK_GENERATED_SRC := 
LIBYACHTROCK_GENERATED_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_GENERATED_SRC))
LIBYACHTROCK_STATIC_SRC := yachtrock.c runtime.c testcase.c results.c yrutil.c multiprocess.c multiprocess_inferior.c multiprocess_superior.c
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

install: install_libyachtrock_dylib install_libyachtrock_headers

install_libyachtrock_dylib: $(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)

$(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_DYLIBNAME) $(PREFIX)/lib
	install -vC $< $(PREFIX)/lib

install_libyachtrock_headers: libyachtrock_headers_installation_dir libyachtrock_installed_headers

libyachtrock_headers_installation_dir: $(PREFIX)/include/yachtrock

$(PREFIX)/include/yachtrock:
	install -dv $(PREFIX)/include/yachtrock

libyachtrock_installed_headers: $(LIBYACHTROCK_HEADER_INSTALLED_FILES)

$(PREFIX)/include/yachtrock/%.h: $(LIBYACHTROCK_DIR)public_headers/yachtrock/%.h
	install -m 0644 -vC $< $(PREFIX)/include/yachtrock

libyachtrock_install_dev_dylib_links: $(PREFIX)/lib
	ln -s `pwd`/$(LIBYACHTROCK $(PREFIX)/lib/$(LIBYACHTROCK_DYLIBNAME)
