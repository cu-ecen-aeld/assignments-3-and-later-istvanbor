CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra -Werror -std=gnu99

# Targets
TARGETS := writer finder
SRC_WRITER := writer.c
SRC_FINDER := finder.c
OBJ_WRITER := $(SRC_WRITER:.c=.o)
OBJ_FINDER := $(SRC_FINDER:.c=.o)

# Default target
.PHONY: all
all: $(TARGETS)

# Build the writer application
writer: $(OBJ_WRITER)
	$(CC) $(CFLAGS) -o $@ $^

# Build the finder application
finder: $(OBJ_FINDER)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
.PHONY: clean
clean:
	rm -f $(TARGETS) $(OBJ_WRITER) $(OBJ_FINDER)

