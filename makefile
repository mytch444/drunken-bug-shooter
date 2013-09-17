game: game.c
	gcc -o game game.c -lcurses -lpthread

clean:
	rm game
