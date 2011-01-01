CC=cc
CCFLAGS=-Wall -pedantic -std=c99 -O2 -g

.PHONY : clean doc bundle

all: xbox_management_server doc

xbox_management_server: src/Server/xbox_management_server.o 
	$(CC) $(CCFLAGS) -o $@ $?

src/Server/xbox_management_server.o: src/Server/xbox_management_server.c
	$(CC) $(CCFLAGS) -o $@ -c $?

bundle:
	tar -czf xbox_management.tar.gz --exclude *.o Makefile Doxyfile README.txt src/*

doc:
	doxygen Doxyfile

clean:
	rm -rf src/Server/*.o
	rm -rf xbox_management_server
	rm -rf *.tar.gz

