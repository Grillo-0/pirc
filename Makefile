COM_BIN = pirc-com
SIMPLE_CLIENT_BIN = pirc-simple-client
SIMPLE_SERVER_BIN = pirc-simple-server

BASE_C_SRCS = $(shell ls src/*.c 2> /dev/null)
COM_C_SRCS = src/com/com.c
SIMPLE_CLIENT_C_SRCS = $(shell ls src/simple_client/*.c 2> /dev/null)
SIMPLE_SERVER_C_SRCS = $(shell ls src/simple_server/*.c 2> /dev/null)

########################################################

# Config flags
CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -ggdb

CFLAGS += $(C_FLAGS)
LDFLAGS += $(LD_FLAGS)
CPPFLAGS += $(CPP_FLAGS)

all : $(SIMPLE_CLIENT_BIN) $(SIMPLE_SERVER_BIN) $(COM_BIN)

$(COM_BIN) : $(BASE_C_SRCS:%.c=%.o) $(COM_C_SRCS:%.c=%.o)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SIMPLE_CLIENT_BIN) : $(BASE_C_SRCS:%.c=%.o) $(SIMPLE_CLIENT_C_SRCS:%.c=%.o)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SIMPLE_SERVER_BIN) : $(BASE_C_SRCS:%.c=%.o) $(SIMPLE_SERVER_C_SRCS:%.c=%.o)
	$(CC) $(CPPFLAGS) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@ $(LDFLAGS)

.PHONY: all clean zip

zip: clean
	zip -r submission src/ Makefile

clean:
	rm -f submission.zip
	rm -f src/*/*.o
	rm -f $(SIMPLE_CLIENT_BIN)
	rm -f $(SIMPLE_SERVER_BIN)
	rm -f $(COM1_BIN)
	rm -f $(COM2_BIN)
