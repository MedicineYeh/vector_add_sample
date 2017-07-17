-include ./config.mak
#get all c files.
SRCS=$(wildcard *.c)
OBJS=$(addprefix objs/, $(patsubst %.c,%.o,$(SRCS)))
DEPS=$(addprefix objs/, $(patsubst %.c,%.d,$(SRCS)))
LOCAL_CFLAGS=-Wno-deprecated-declarations
TARGET=cl_test

.PHONY: all clean execute directories

all	:	directories $(TARGET)

$(TARGET)	:	$(OBJS)
	@echo "  LINK  $@"
	@$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS) $(LOCAL_CFLAGS)

objs/%.o	:	%.c
	@echo "  CC    $@"
	@$(CC) -c $< -o $@ $(CFLAGS) $(LOCAL_CFLAGS)

objs/%.d	:	%.c
	@$(CC) -MM $^ -MT $(patsubst %.d,%.o,$@) > $@

directories	:	
	@mkdir -p objs

clean	:	
	rm -f objs/* $(TARGET)

execute	:	all
	./$(TARGET)

# Include automatically generated dependency files
-include $(DEPS)
