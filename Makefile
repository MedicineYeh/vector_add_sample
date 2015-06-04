-include ./config.mak
#get all c files.
SRCS=$(wildcard *.c)
OBJS=$(addprefix objs/, $(patsubst %.c,%.o,$(SRCS)))
DEPS=$(addprefix objs/, $(patsubst %.c,%.d,$(SRCS)))
LOCAL_CFLAGS=
TARGET=cl_test

.PHONY: all clean execute directories

all	:	directories $(TARGET)

$(TARGET)	:	$(OBJS)
	@echo "  LINK  $@"
	@$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS) $(LOCAL_CFLAGS)

objs/%.o	:	%.c
	@echo "  CC    $@"
	@$(CC) -c $< -o $@ $(CFLAGS) $(LOCAL_CFLAGS)

%.d	:	%.c
	@$(CC) -MM $(VPMU_CFLAGS) $^ > $@

directories	:	
	@mkdir -p objs

clean	:	
	rm objs/* $(TARGET)

execute	:	all
	./$(TARGET)

# Include automatically generated dependency files
-include $(DEPS)
