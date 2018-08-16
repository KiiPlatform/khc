
TARGET=./testapp
INCLUDES=-I.
LIBS=-lkiihttp
SRCS=$(wildcard *.cpp)
TEST_RESULT=test-result.xml

$(TARGET): $(SRCS)
	g++ -g $(SRCS) $(LIBS) $(INCLUDES) -o $(TARGET) -std=c++11

test: $(TARGET)
	$(TARGET) -r junit -o $(TEST_RESULT)

memcheck: $(TARGET)
	valgrind --leak-check=yes $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(TEST_RESULT)

.PHONY: clean test memcheck
