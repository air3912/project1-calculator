CXX = c++
CPPFLAGS = -Iinclude
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
BUILD = build
CORE = src/polynomial.cpp src/expression.cpp
HEADERS = $(wildcard include/calculator/*.hpp)

.PHONY: all test sanitize clean

all: $(BUILD)/calculator $(BUILD)/libcalculator.a

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/polynomial.o: src/polynomial.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/expression.o: src/expression.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/libcalculator.a: $(BUILD)/polynomial.o $(BUILD)/expression.o
	$(AR) rcs $@ $^

$(BUILD)/calculator: src/main.cpp $(BUILD)/libcalculator.a $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(BUILD)/libcalculator.a -o $@

$(BUILD)/tests: tests/test_calculator.cpp $(BUILD)/libcalculator.a $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_calculator.cpp $(BUILD)/libcalculator.a -o $@

test: $(BUILD)/tests
	./$(BUILD)/tests

sanitize: | $(BUILD)
	$(CXX) $(CPPFLAGS) -std=c++17 -Wall -Wextra -Wpedantic -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined $(CORE) tests/test_calculator.cpp -o $(BUILD)/tests-sanitize
	./$(BUILD)/tests-sanitize

clean:
	rm -rf $(BUILD)
