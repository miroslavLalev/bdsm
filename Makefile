ifndef CC
	CC=gcc
endif
CFLAGS=-std=c99 -Wall -Wpedantic -Wextra # TODO: add -Werror
SRCS=bdsm.c fs.c sblock_ops.c mblock_ops.c layout_ops.c encutils.c inode_ops.c inode_vec_ops.c inode.c
OBJS=$(subst .c,.o,$(SRCS))
RM=rm -f

bdsm: ${OBJS}
	${CC} -o bdsm ${CFLAGS} ${OBJS} -lm
	${RM} ${OBJS}

all: bdsm

#foo: main.o
#	$(CC) $(CFLAGS) -o main main.c

clean:
	$(RM) $(OBJS) bdsm
