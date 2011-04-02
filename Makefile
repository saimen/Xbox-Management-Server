CC?=cc
CCFLAGS+=-Wall -Wextra -Wformat=2 -Winit-self -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wtrigraphs -ansi -pedantic-errors -std=c99 -O2
DEBUG+=-Wall -Wextra -Wformat=2 -Winit-self -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wtrigraphs -ansi -pedantic-errors -std=c99 -DTEST -g
LIBS+=-lpthread

.PHONY : clean doc bundle

all: xbox_management_server 

xbox_management_server: src/Server/xbox_management_server.o 
	$(CC) $(CCFLAGS) -o $@ $? $(LIBS)

src/Server/xbox_management_server.o: src/Server/xbox_management_server.c
	$(CC) $(CCFLAGS) -o $@ -c $?

debug: xbox_management_server_debug

xbox_management_server_debug: src/Server/xbox_management_server_debug.o
	$(CC) $(DEBUG) -o $@ $? $(LIBS)

src/Server/xbox_management_server_debug.o: src/Server/xbox_management_server.c
	$(CC) $(DEBUG) -o $@ -c $?

memcheck: xbox_management_server_debug
	valgrind --tool=memcheck --leak-check=full --show-reachable=yes --leak-resolution=high --track-origins=yes -v --log-file=$@.log ./$?

helgrind: xbox_management_server_debug
	valgrind --tool=helgrind --track-fds=yes --track-lockorders=yes --history-level=full -v --log-file=$@.log ./$?

bundle:
	tar -czf xbox_management.tar.gz --exclude=*.o --exclude=.* --exclude=*DS_Store* Makefile Doxyfile README.txt src/*

doc:
	doxygen Doxyfile

clean:
	rm -rf src/Server/*.o xbox_management_server* *.tar.gz doc/* *.log


