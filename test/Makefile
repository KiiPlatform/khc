
TARGET=./testapp
INCLUDES=-I.
LIBS=-lkiihttp
all: clean $(TARGET)

$(TARGET):
	g++ mock_test.cpp $(LIBS) $(INCLUDES) -o $(TARGET) -std=c++11

test: $(TARGET)
	$(TARGET) -r junit -o unit-test.xml

memory-check: $(TARGET)
	valgrind --leak-check=yes $(TARGET)
clean:
	touch $(TARGET)
	rm $(TARGET)
	touch unit-test.xml
	rm unit-test.xml

.PHONY: all clean test memory-check
