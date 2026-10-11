CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -g

SRC_DIR = src
TEST_DIR = tests
INC_DIR = include

SRCS = $(SRC_DIR)/packet_parser.c $(TEST_DIR)/test_parser.c
TARGET = test_runner

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS) 
			$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

clean: 
	rm -f $(TARGET)