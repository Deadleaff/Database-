prog : main.o methods.o header2.h
	g++ -o prog main.o methods.o

main.o : main.cpp header2.h
	g++ -c -o main.o main.cpp

methods.o : methods.cpp header2.h
	g++ -c -o methods.o methods.cpp
