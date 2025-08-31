CC = gcc
CFLAGS = -Wall

SRC_FILES = simplefs-ops.c simplefs-disk.c

TEST_CASES = testcase0 testcase1 testcase2 testcase3 testcase4 testcase5 testcase6 testcase7

.PHONY: all clean

all: $(TEST_CASES)

# Found usage of $@ and $^ online
# $@ is replaced by the target name while $^ is replaced by the dependencies
testcase0: testcase0.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^     

testcase1: testcase1.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase2: testcase2.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase3: testcase3.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase4: testcase4.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase5: testcase5.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase6: testcase6.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

testcase7: testcase7.c $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_CASES) simplefs