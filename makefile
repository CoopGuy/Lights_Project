dirs = pigpio-master IRDecode
libs = pigpio rt IRDecode
args = -Wall -Werror -pthread
CC = gcc

Lights : Lights.o
	$(CC) $(args) -o Lights Lights.o $(dirs:%=-L%) $(libs:%=-l%)

Lights.o : Lights.c
	$(CC) $(args) -c Lights.c $(dirs:%=-I%) 

clean :
	rm Lights.o
