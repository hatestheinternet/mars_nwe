CFLAGS=-Iinclude -ggdb -Wall -Werror
SRCS=src/mars_minwe.c \
	src/config.c \
	src/network.c \
	src/ipx_utils.c \
	src/router.c \
	src/rip.c \
	src/sap.c \
	src/server.c \
	src/bindery.c \
	src/ncp.c \
	src/ncp_conn.c \
	src/ncp_svc.c \
	src/ncp_fserv.c \
	src/nds.c
OBJS=$(patsubst %.c, %.o, $(SRCS))

LIBS=-lpthread

default: all

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

all: $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o mars_minwe $(LIBS)

clean:
	rm -f mars_minwe $(OBJS)
