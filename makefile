game: game.c
	gcc -o drunken-bug-shooter game.c -lcurses -lpthread

clean:
	rm game
