CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Iinclude

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
TARGET := load-balancer

.PHONY: all check clean docs package

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

docs:
	doxygen Doxyfile

check: $(TARGET)
	python3 tests/check_summary.py

package:
	mkdir -p dist
	zip -q -FS -r dist/CodeHarbor_Project_4_Source.zip Makefile config.txt \
		Doxyfile README.md include src tests/check_summary.py \
		docs/mainpage.md docs/codeharbor.css

clean:
	rm -rf build $(TARGET) docs/html
