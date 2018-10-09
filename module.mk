LIBYACHTROCK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
LIBYACHTROCK_DYLIBNAME := libyachtrock.dylib
else
LIBYACHTROCK_DYLIBNAME := libyachtrock.so
endif

PRODUCTS += $(LIBYACHTROCK_DYLIBNAME)
TESTS += test_libyachtrock

MODULE_CLEANS += clean_libyachtrock

LIBYACHTROCK_GENERATED_SRC := 
LIBYACHTROCK_GENERATED_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_GENERATED_SRC))
LIBYACHTROCK_STATIC_SRC := yachtrock.c runtime.c testcase.c results.c
LIBYACHTROCK_STATIC_SRC := $(patsubst %,$(LIBYACHTROCK_DIR)src/%,$(LIBYACHTROCK_STATIC_SRC))
LIBYACHTROCK_SRC := $(LIBYACHTROCK_STATIC_SRC) $(LIBYACHTROCK_GENERATED_SRC)
LIBYACHTROCK_HEADER_SOURCES := $(wildcard $(LIBYACHTROCK_DIR)public_headers/yachtrock/*.h)
LIBYACHTROCK_HEADER_INSTALLED_FILES := $(subst $(LIBYACHTROCK_DIR)public_headers,$(PREFIX)/include,$(LIBYACHTROCK_HEADER_SOURCES))

LIBYACHTROCK_TESTSRC := basic_tests.c result_store_tests.c
LIBYACHTROCK_TESTSRC := $(patsubst %,$(LIBYACHTROCK_DIR)test/%,$(LIBYACHTROCK_TESTSRC))
CSRC += $(LIBYACHTROCK_SRC)
CSRC += $(LIBYACHTROCK_TESTSRC)
LIBYACHTROCK_OBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_SRC)))
LIBYACHTROCK_TESTOBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_TESTSRC)))
LIBYACHTROCK_TESTS = $(patsubst %.o,%,$(LIBYACHTROCK_TESTOBJ))
LIBYACHTROCK_LINKS =

$(LIBYACHTROCK_STATIC_SRC): $(LIBYACHTROCK_GENERATED_SRC)

$(LIBYACHTROCK_OBJ): CFLAGS += -fPIC

test_libyachtrock: test_libyachtrock_basic test_libyachtrock_result_store

test_libyachtrock_%: libyachtrock_%_tests_success
	true

libyachtrock_%_tests_success: $(LIBYACHTROCK_DIR)test/%_tests
	./$<

$(LIBYACHTROCK_DIR)test/%_tests: $(LIBYACHTROCK_DIR)test/%_tests.o libyachtrock.a
	cc $^ $(LIBYACHTROCK_LINKS) -o $@

# without this the test binaries themselves are considered intermediaries and are removed
.SECONDARY: $(LIBYACHTROCK_TESTS)

libyachtrock.a: $(LIBYACHTROCK_OBJ)
	ar rc $@ $(LIBYACHTROCK_OBJ)
	ranlib $@

ifeq ($(UNAME_S),Darwin)
$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_OBJ)
	cc -dynamiclib $(LIBYACHTROCK_OBJ) -install_name $(PREFIX)/lib/$@ $(LIBYACHTROCK_LINKS) -o $@
else
$(LIBYACHTROCK_DYLIBNAME): $(LIBYACHTROCK_OBJ)
	cc -shared $(LIBYACHTROCK_OBJ) $(LIBYACHTROCK_LINKS) -Wl,-rpath,$(PREFIX)/lib/$@ -o $@
endif

clean_libyachtrock:
	rm -f libyachtrock.a
	rm -f $(LIBYACHTROCK_DYLIBNAME)
	rm -f $(LIBYACHTROCK_OBJ)
	rm -f $(LIBYACHTROCK_TESTOBJ)
	rm -f $(LIBYACHTROCK_TESTS)
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
