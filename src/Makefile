TARGET = mysh
CC     = gcc
CFLAGS = -g -std=c99 -Wall -Wvla -Werror -fsanitize=address

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM