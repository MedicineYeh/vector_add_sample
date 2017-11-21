#!/usr/bin/make

# Name of generated binary
TARGET=cl_test
# All your source codes to build the target
SRCS=main.c
# Automatic generate other deps
OBJS=$(addprefix objs/, $(patsubst %.c,%.o,$(SRCS)))
DEPS=$(addprefix objs/, $(patsubst %.c,%.d,$(SRCS)))

CC=gcc
CFLAGS=-g -Wall
LFLAGS=-lOpenCL
EXTRA_CFLAGS=-Wno-deprecated-declarations

.PHONY: all clean execute directories

all : directories $(TARGET)


execute : all
	./$(TARGET)

clean :
	rm -f objs/* $(TARGET)

directories :
	@mkdir -p objs

$(TARGET) : $(OBJS)
	@echo "  LINK  $@"
	@$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS) $(EXTRA_CFLAGS)

objs/%.o : %.c
	@echo "  CC    $@"
	@$(CC) -c $< -o $@ $(CFLAGS) $(EXTRA_CFLAGS)

objs/%.d : %.c
	@$(CC) -MM $^ -MT $(patsubst %.d,%.o,$@) > $@

# Include automatically generated dependency files
-include $(DEPS)
