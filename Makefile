TARGET = Test_task
PREFIX = /usr/local/bin

.PHONY: all clean install uninstall

all: $(TARGET)
	
clean:
			rm -rf $(TARGET) *.o
main.o: main.c
			gcc -c -o main.o main.c -pthread
$(TARGET): main.o
			gcc -o $(TARGET) main.o -pthread
install:
			install $(TARGET) $(PREFIX)
uninstall:
			rm -rf $(PREFIX)/$(TARGET)
