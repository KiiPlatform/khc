
TARGET=./testapp
INCLUDES=-I. -I../
all: clean $(TARGET)

$(TARGET):
	g++ slist_test.cpp ../kii_http.c $(INCLUDES) -o $(TARGET) -std=c++11

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