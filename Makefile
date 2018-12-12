# Make file for building the four tools

CFLAGS= -DNDEBUG -g -Wall
LDLIBS= -lm -pthread
CC=gcc

all: diskinfo disklist diskput diskget
	echo All executable done

diskinfo: diskinfo.o ADTlinkedlist.o utils.o FATheaders.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o diskinfo
	
disklist: disklist.o ADTlinkedlist.o utils.o FATheaders.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o disklist

diskput:diskput.o  ADTlinkedlist.o utils.o FATheaders.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o diskput

diskget: diskget.o ADTlinkedlist.o utils.o FATheaders.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o diskget

%.o: %.c
	$(CC) -c $(LDLIBS) $(CFLAGS) $^
	
clean:
	rm -f *.o *.gch diskget diskput disklist diskinfo

debug:
	$(MAKE) CFLAGS='-Wextra -pedantic-errors -fsanitize=address -Wall -g'
