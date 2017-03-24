CC=g++
CFLAGS=-Wall  

all:
	mkdir -p build/release
	$(CC) $(CFLAGS) -O3 -c boggle.cpp
	$(CC) $(CFLAGS) -O3 -c main.cpp
	$(CC) main.o boggle.o -o build/release/boggle
	rm -f *.o

debug:
	mkdir -p build/debug
	$(CC) $(CFLAGS) -g -c -D_DEBUG boggle.cpp
	$(CC) $(CFLAGS) -g -c -D_DEBUG main.cpp
	$(CC) main.o boggle.o -o build/debug/boggle
	rm -f *.o


perf:
	mkdir -p build/release
	$(CC) $(CFLAGS) -fno-omit-frame-pointer -O3 -c boggle.cpp
	$(CC) $(CFLAGS) -fno-omit-frame-pointer -O3 -c main.cpp
	$(CC) main.o boggle.o -o build/release/boggle
	rm -f *.o