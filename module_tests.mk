# -*- mode: makefile-gmake -*-

test: test_libyachtrock

LIBYACHTROCK_TESTSRC := selftests_collection.c basic_tests.c result_store_tests.c assertion_tests.c testcase_tests.c run_under_store_tests.c selector_tests.c
ifeq ($(YACHTROCK_MULTIPROCESS),1)
LIBYACHTROCK_TESTSRC += multiprocess_basic_tests.c
endif
LIBYACHTROCK_TESTSRC := $(patsubst %,$(LIBYACHTROCK_DIR)test/%,$(LIBYACHTROCK_TESTSRC))
LIBYACHTROCK_TESTMAIN = $(LIBYACHTROCK_DIR)test/test_main.c
CSRC += $(LIBYACHTROCK_TESTSRC) $(LIBYACHTROCK_TESTSUPPORT_SRC) $(LIBYACHTROCK_TESTMAIN)
LIBYACHTROCK_TESTOBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_TESTSRC)))
LIBYACHTROCK_TESTMAIN_OBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_TESTMAIN)))
LIBYACHTROCK_TESTSUPPORT =
LIBYACHTROCK_TESTSUPPORT_OBJ =


YR_SIMPLE_RUNNER = $(LIBYACHTROCK_DIR)test/yachtrock_test_runner

ifeq ($(YAP_UNAME_S), Darwin)
YR_TEST_DYLIB = $(LIBYACHTROCK_DIR)test/libyachtrock_test.dylib
else
YR_TEST_DYLIB = $(LIBYACHTROCK_DIR)test/libyachtrock_test.so
endif

YR_SELFTESTS_DYLIB = $(LIBYACHTROCK_DIR)test/libyachtrock_selftests.dylib
YR_RUNTESTS_TEST = $(LIBYACHTROCK_DIR)test/yr_runtests_test
YR_MULTIPROCESS_TEST_TRAMPOLINE = $(LIBYACHTROCK_DIR)test/multiprocess_test_trampoline

$(LIBYACHTROCK_TESTOBJ): CFLAGS += -D_POSIX_C_SOURCE=200809L

clean_libyachtrock_tests:
	rm -f $(LIBYACHTROCK_TESTOBJ) $(LIBYACHTROCK_TESTMAIN_OBJ)
	rm -f $(LIBYACHTROCK_TESTS)
	rm -f $(LIBYACHTROCK_TESTSUPPORT)
	rm -f $(LIBYACHTROCK_TESTSUPPORT_OBJ)
	rm -f $(YR_SIMPLE_RUNNER)
	rm -f $(YR_TEST_DYLIB)
	rm -f $(YR_RUNTESTS_TEST)
	rm -f $(YR_SELFTESTS_DYLIB)

clean_libyachtrock: clean_libyachtrock_tests

YACHTROCK_TEST_INVOCATION_ENVIRON =

$(YR_TEST_DYLIB): $(LIBYACHTROCK_OBJ)
	$(YAP_LINK) --driver $(CC) --dynamic_install_name `pwd`/$@ $(LIBYACHTROCK_YAP_LINK_OPTIONS) -- $(LIBYACHTROCK_OBJ) $(LIBYACHTROCK_LINKS) -o $@

YR_TEST_DYLIB_LDFLAGS = -L$(LIBYACHTROCK_DIR)test -lyachtrock_test

$(YR_SELFTESTS_DYLIB): $(LIBYACHTROCK_TESTOBJ) $(YR_TEST_DYLIB)
	$(YAP_LINK) --driver $(CC) --dynamic_install_name `pwd`/$@ --dl -- $(LIBYACHTROCK_TESTOBJ) $(YR_TEST_DYLIB_LDFLAGS) -o $@

$(YR_RUNTESTS_TEST): $(YR_RUNTESTS_OBJ) $(YR_TEST_DYLIB)
	$(YAP_LINK) --driver $(CC) $(LIBYACHTROCK_YAP_LINK_OPTIONS) -- $(YR_TEST_DYLIB_LDFLAGS) $(YR_RUNTESTS_OBJ) -o $@

$(YR_MULTIPROCESS_TEST_TRAMPOLINE): $(LIBYACHTROCK_DIR)test/multiprocess_basic_tests.o $(LIBYACHTROCK_DIR)test/multiprocess_test_trampoline.o $(YR_TEST_DYLIB)
	$(YAP_LINK) --driver $(CC) -- $(LIBYACHTROCK_DIR)test/multiprocess_basic_tests.o $(LIBYACHTROCK_DIR)test/multiprocess_test_trampoline.o $(YR_TEST_DYLIB_LDFLAGS) $(LIBYACHTROCK_LINKS) -o $@

_yr_clean_trampoline:
	rm -f $(YR_MULTIPROCESS_TEST_TRAMPOLINE) $(LIBYACHTROCK_DIR)test/multiprocess_test_trampoline.o
clean_libyachtrock: _yr_clean_trampoline

$(YR_SIMPLE_RUNNER): $(LIBYACHTROCK_TESTMAIN_OBJ) $(LIBYACHTROCK_TESTOBJ) $(YR_TEST_DYLIB)
	$(YAP_LINK) --driver $(CC) --dl -- $(LIBYACHTROCK_TESTMAIN_OBJ) $(LIBYACHTROCK_TESTOBJ) $(YR_TEST_DYLIB_LDFLAGS) $(LIBYACHTROCK_LINKS) -o $@

YR_TEST_CONDITIONAL_DEPENDENCIES :=

ifeq ($(YACHTROCK_MULTIPROCESS),1)
YACHTROCK_TEST_INVOCATION_ENVIRON += YR_MULTIPROCESS_TEST_TRAMPOLINE=$(YR_MULTIPROCESS_TEST_TRAMPOLINE)
YR_TEST_CONDITIONAL_DEPENDENCIES += $(YR_MULTIPROCESS_TEST_TRAMPOLINE)
endif

ifeq ($(YACHTROCK_DLOPEN),1)
$(LIBYACHTROCK_TESTOBJ): CFLAGS += -fpic
YACHTROCK_TEST_INVOCATION_ENVIRON += YRTEST_DUMMY_MODULE=$(LIBYACHTROCK_DIR)test/dummy_module.dylib

$(LIBYACHTROCK_DIR)test/dummy_module.o: CFLAGS += -fPIC -D_POSIX_C_SOURCE=200809L
$(LIBYACHTROCK_DIR)test/dummy_module.dylib: $(LIBYACHTROCK_DIR)test/dummy_module.o $(YR_TEST_DYLIB)
	$(YAP_LINK) --driver $(CC) --dynamic_install_name `pwd`/$@ -- $(LIBYACHTROCK_DIR)test/dummy_module.o $(YR_TEST_DYLIB_LDFLAGS) -o $@

YR_TEST_CONDITIONAL_DEPENDENCIES += $(LIBYACHTROCK_DIR)test/dummy_module.dylib
test_libyachtrock: test_libyachtrock_via_selftest_dylib

LIBYACHTROCK_TESTSUPPORT += $(LIBYACHTROCK_DIR)test/dummy_module.dylib
LIBYACHTROCK_TESTSUPPORT_SRC = $(patsubst %,$(LIBYACHTROCK_DIR)test/%,dummy_module.c)
LIBYACHTROCK_TESTSUPPORT_OBJ = $(patsubst %.c,%.o,$(LIBYACHTROCK_TESTSUPPORT_SRC))
CSRC += $(LIBYACHTROCK_TESTSUPPORT_SRC)
endif

test_libyachtrock_via_selftest_dylib: $(YR_RUNTESTS_TEST) $(YR_SELFTESTS_DYLIB) $(YR_TEST_CONDITIONAL_DEPENDENCIES)
	$(YACHTROCK_TEST_INVOCATION_ENVIRON) $(YR_RUNTESTS_TEST) -n "yachtrock self-tests from dylib"  $(YR_SELFTESTS_DYLIB)

test_libyachtrock_via_simple_runner: $(YR_SIMPLE_RUNNER) $(YR_TEST_CONDITIONAL_DEPENDENCIES)
	$(YACHTROCK_TEST_INVOCATION_ENVIRON) $(YR_SIMPLE_RUNNER)

test_libyachtrock: test_libyachtrock_via_simple_runner

