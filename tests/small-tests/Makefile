LIB_SRCS=$(wildcard ../../src/*.c)
LIB_SRCS+=$(wildcard ../../src/*.h)
LIB_SRCS+=$(wildcard ../../include/*.h)
LIB_BUILD_DIR=build-http

TARGET=./testapp
INCLUDES=-I. -I.. -I../../src -I$(LIB_BUILD_DIR)/include
LIBS_DIR=-L$(LIB_BUILD_DIR)/lib

LIBS=-lkhc
SRCS=$(wildcard *.cpp)
SRCS+=$(wildcard ../*.cpp)
TEST_RESULT_DIR=test-results/junit
TEST_RESULT=$(TEST_RESULT_DIR)/test-result.xml

ifdef MEMCHECK
TESTCMD=valgrind --leak-check=yes $(TARGET)
else
TESTCMD=$(TARGET)
endif

ifdef JUNIT
TESTCMD+= -r junit -o $(TEST_RESULT)
endif

all: clean test

$(LIB_BUILD_DIR): $(LIB_SRCS)
	mkdir -p $(LIB_BUILD_DIR)
	cd $(LIB_BUILD_DIR) && cmake -DCMAKE_INSTALL_PREFIX=. ../../.. && make install

$(TARGET): $(SRCS) $(LIB_BUILD_DIR)
	g++ -g $(SRCS) $(LIBS_DIR) $(LIBS) $(INCLUDES) -o $(TARGET) -std=c++11

build: $(TARGET)

test: $(TARGET)
	mkdir -p $(TEST_RESULT_DIR)
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(LIB_BUILD_DIR)/lib \
	$(TESTCMD)

clean:
	rm -f $(TARGET)
	rm -rf $(TEST_RESULT_DIR)
	rm -rf $(LIB_BUILD_DIR)

.PHONY: build clean test
