CC?=cc
CCFLAGS+=-Wall -Wextra -Wformat=2 -Winit-self -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wtrigraphs -ansi -pedantic-errors -std=c99 -O2 -g
DEBUG+=-DTEST
LIBS+=-lpthread

.PHONY : clean doc bundle

all: xbox_management_server doc

xbox_management_server: src/Server/xbox_management_server.o 
	$(CC) $(CCFLAGS) -o $@ $? $(LIBS)

src/Server/xbox_management_server.o: src/Server/xbox_management_server.c
	$(CC) $(CCFLAGS) -o $@ -c $?

bundle:
	tar -czf xbox_management.tar.gz --exclude=*.o --exclude=.* --exclude=*DS_Store* Makefile Doxyfile README.txt src/*

doc:
	doxygen Doxyfile

clean:
	rm -rf src/Server/*.o xbox_management_server *.tar.gz doc/*

