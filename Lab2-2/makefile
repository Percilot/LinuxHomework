CC = gcc
HEADERS = main.h input.h compute.h
SOURCES = main.c input.c compute.c
OBJECTS = main.o input.o compute.o
power: $(OBJECTS)
	$(CC) $(OBJECTS) -o power -lm
main.o: main.c $(HEADERS)
	$(CC) -c main.c -o main.o
input.o: input.c input.h
	$(CC) -c input.c -o input.o
compute.o: compute.c compute.h
	$(CC) -c compute.c -o compute.o