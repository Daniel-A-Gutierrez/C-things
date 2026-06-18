CC       := clang
CFLAGS   := $(shell cat compile_flags.txt | tr '\n' ' ')
TARGET   := target/lsbtw
SRC      := src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: clean