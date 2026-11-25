CC = cc
CFLAGS = -Wall -Wextra

sample: main.o sqlite3.o
	$(CC) main.o sqlite3.o -o sample

main.o: main.c ../cwf.h
	$(CC) $(CFLAGS) -c main.c -o main.o

sqlite3.o: sqlite3.c sqlite3.h
	$(CC) $(CFLAGS) -c sqlite3.c -o sqlite3.o

clean:
	rm -f ./*.o
