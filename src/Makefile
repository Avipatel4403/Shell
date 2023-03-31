TARGET = mysh
CC     = gcc
CFLAGS = -g -std=c99 -Wall -Werror -fsanitize=address,undefined

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM