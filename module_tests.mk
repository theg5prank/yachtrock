TESTS += test_libyachtrock

LIBYACHTROCK_TESTSRC := basic_tests.c result_store_tests.c assertion_tests.c testcase_tests.c dummy_module.c run_under_store_tests.c multiprocess_basic_tests.c
LIBYACHTROCK_TESTSRC := $(patsubst %,$(LIBYACHTROCK_DIR)test/%,$(LIBYACHTROCK_TESTSRC))
CSRC += $(LIBYACHTROCK_TESTSRC)
LIBYACHTROCK_TESTOBJ = $(patsubst %.c,%.o,$(filter %.c,$(LIBYACHTROCK_TESTSRC)))
LIBYACHTROCK_TESTS = $(patsubst %.o,%,$(LIBYACHTROCK_TESTOBJ))
LIBYACHTROCK_TESTSUPPORT =

$(LIBYACHTROCK_TESTOBJ): CFLAGS += -D_POSIX_C_SOURCE=200809L

clean_libyachtrock_tests:
	rm -f $(LIBYACHTROCK_TESTOBJ)
	rm -f $(LIBYACHTROCK_TESTS)
	rm -f $(LIBYACHTROCK_TESTSUPPORT)

clean_libyachtrock: clean_libyachtrock_tests

test_libyachtrock: test_libyachtrock_basic test_libyachtrock_result_store test_libyachtrock_assertion test_libyachtrock_testcase test_libyachtrock_run_under_store test_libyachtrock_multiprocess_basic

test_libyachtrock_%: libyachtrock_%_tests_success
	true

libyachtrock_%_tests_success: $(LIBYACHTROCK_DIR)test/%_tests
	./$<

$(LIBYACHTROCK_DIR)test/dummy_module.o: CFLAGS += -fPIC
ifeq ($(UNAME_S),Darwin)
$(LIBYACHTROCK_DIR)test/dummy_module.dylib: $(LIBYACHTROCK_DIR)test/dummy_module.o $(LIBYACHTROCK_ARNAME)
	$(CC) -dynamiclib $^ -o $@
else
$(LIBYACHTROCK_DIR)test/dummy_module.dylib: $(LIBYACHTROCK_DIR)test/dummy_module.o $(LIBYACHTROCK_ARNAME)
	$(CC) -shared $^ -o $@
endif
libyachtrock_testcase_tests_success: $(LIBYACHTROCK_DIR)test/testcase_tests $(LIBYACHTROCK_DIR)test/dummy_module.dylib
	$^
LIBYACHTROCK_TESTSUPPORT += $(LIBYACHTROCK_DIR)test/dummy_module.dylib

$(LIBYACHTROCK_DIR)test/%_tests: $(LIBYACHTROCK_DIR)test/%_tests.o $(LIBYACHTROCK_ARNAME)
	$(CC) $^ $(LIBYACHTROCK_LINKS) -o $@

# without this the test binaries themselves are considered intermediaries and are removed
.SECONDARY: $(LIBYACHTROCK_TESTS)

