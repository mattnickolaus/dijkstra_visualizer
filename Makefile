CC=gcc
CFLAGS=-Wall -g
RAYLIBFLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

dijkstras: dijkstras.c
	$(CC) $(CFLAGS) -o dijkstras dijkstras.c

gui: gui.c
	$(CC) gui.c dijkstras.c $(CFLAGS) $(RAYLIBFLAGS) -o gui 

web: gui.c
	emcc gui.c dijkstras.c -o ./web/dikjstra.html -Wall -Os -I. -I$(HOME)/raylib/src -I$(HOME)/raylib/src/external -L$(HOME)/raylib/src -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --shell-file $(HOME)/raylib/src/minshell.html $(HOME)/raylib/src/libraylib.web.a -DPLATFORM_WEB

# emcc gui.c dijkstras.c -o dikjstra.html -Wall -Os -I. -I$(HOME)/raylib/src -I$(HOME)/raylib/src/external -L$(HOME)/raylib/src -lraylib -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --shell-file $(HOME)/raylib/src/shell.html -DPLATFORM_WEB

	# emcc -o dijstra.html gui.c dijkstras.c -Os -Wall ~/raylib/src/libraylib.a -I. -I~/raylib/src/ -L. -L~/raylib/src/libraylib.a -s USE_GLFW=3 -s ASYNCIFY --shell-file ~/raylib/src/shell.html -DPLATFORM_WEB

# emcc -o snake_c.html main.c -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces -Wunused-result -Os -I. -I C:/raylib/raylib/src -I C:/raylib/raylib/src/external -L. -L C:/raylib/raylib/src -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --shell-file C:/raylib/raylib/src/shell.html C:/raylib/raylib/src/web/libraylib.a -DPLATFORM_WEB -s 'EXPORTED_FUNCTIONS=["_free","_malloc","_main"]'-s EXPORTED_RUNTIME_METHODS=ccall


