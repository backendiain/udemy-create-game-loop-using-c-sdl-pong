build:
	gcc -Wall -std=c99 ./src/*.c `sdl2-config --cflags --libs` -o pong

run: 
	./pong

clean:
	rm pong