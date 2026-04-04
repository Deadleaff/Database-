# Обычная сборка
prog : main.o methods.o header2.h
	g++ -o prog main.o methods.o

# Сборка с ASan
prog_asan : main_asan.o methods_asan.o header2.h
	g++ -fsanitize=address -g -o prog_asan main_asan.o methods_asan.o

main.o : main.cpp header2.h
	g++ -c -o main.o main.cpp

main_asan.o : main.cpp header2.h
	g++ -fsanitize=address -g -c -o main_asan.o main.cpp

methods.o : methods.cpp header2.h
	g++ -c -o methods.o methods.cpp

methods_asan.o : methods.cpp header2.h
	g++ -fsanitize=address -g -c -o methods_asan.o methods.cpp

clean:
	rm -f prog prog_asan *.o

.PHONY: clean