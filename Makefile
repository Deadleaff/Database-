# Обычная сборка
server : server.o methods.o header2.h
	g++ -o server server.o methods.o



server.o : server.cpp header2.h
	g++ -c -o server.o server.cpp



methods.o : methods.cpp header2.h
	g++ -c -o methods.o methods.cpp



clean:
	rm -f server prog_asan *.o

.PHONY: clean
