all: test/variant_test

test/variant_test: test/variant_test.o test/variant_test_main.o
	$(CXX) $^ -o $@

%.o: %.cpp
	$(CXX) $< -o $@ -c -std=c++14 -I.

test:
	test/variant_test

.PHONY: test
