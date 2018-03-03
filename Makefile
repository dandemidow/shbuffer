default: all

LIB_NAME = libshbuffer.so

BLDIR = .build

LIB_TARGET = ${BLDIR}/${LIB_NAME}

CC_NAME = gcc
RM = rm

COMMON_FLAGS := \
  -Wall \
  -Wextra \
  -Wundef \
  -Werror \
  -std=c99 \
  -ffunction-sections \
  -fPIC
  
OPT := -Os

CC_FLAGS = $(COMMON_FLAGS)
CC_SYMBOLS := -D_XOPEN_SOURCE=500
 
INCLUDES += -I$(PWD)/include

CFLAGS = $(CC_FLAGS) $(CC_SYMBOLS) $(INCLUDES)

SRCS := $(wildcard $(PWD)/src/*.c)
TEST_SRCS := $(wildcard $(PWD)/src/tst/*.c)

OBJS := $(patsubst $(PWD)%,${BLDIR}%,$(SRCS:%.c=%.o))
TEST_OBJS := $(patsubst $(PWD)%,${BLDIR}%,$(TEST_SRCS:%.c=%.o))

LDFLAGS = -L$(BLDIR)


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),mkdbg)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

$(BLDIR)/%.o : $(PWD)/%.c
	echo $@
	@mkdir -p $(dir $@)
	$(CC_NAME) $(OPT) $(CC_FLAGS) $(CC_SYMBOLS) $(INCLUDES) -c $< -o $@

$(LIB_TARGET): $(OBJS)
	$(CC_NAME) -shared $(LDFLAGS) -o $@ $^
	
test: $(TEST_OBJS)
	$(CC_NAME) $(LDFLAGS) -lshbuffer -lcheck -pthread -o $@ $^
	
all: $(OBJS) $(LIB_TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(LIB_NAME)
	$(RM) -r $(BLDIR)

