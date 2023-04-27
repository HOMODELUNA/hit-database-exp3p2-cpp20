
all:
	cd src
	g++ -std=c++20 *.cpp exceptions/*.cpp -I. -Wall -o badgerdb_main

clean:
	cd src;\
	rm -f badgerdb_main test.?

doc:
	doxygen Doxyfile

test: 
	cd src
	./badgerdb_main.exe
