
CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra -Werror -std=gnu99

# Targets
TARGET := writer
SRC := writer.c
OBJ := $(SRC:.c=.o)

# Default target
.PHONY: all
all: $(TARGET)

# Build the application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

