CC=gcc
CFLAGS=-Wall -g
RAYLIBFLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

dijkstras: dijkstras.c
	$(CC) $(CFLAGS) -o dijkstras dijkstras.c

gui: gui.c
	$(CC) gui.c dijkstras.c $(CFLAGS) $(RAYLIBFLAGS) -o gui 
