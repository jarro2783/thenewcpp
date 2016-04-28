all: test/variant_test

test/variant_test: test/variant_test.o
	$(CXX) $< -o $@

test/variant_test.o: test/variant_test.cpp
	$(CXX) $< -o $@ -c -std=c++14 -I.

.PHONY: test
