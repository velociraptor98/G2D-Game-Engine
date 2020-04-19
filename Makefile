build:
	g++ -w -std=c++14 -Wfatal-errors -m32 -static-libstdc++ \
	./src/*.cpp \
	-o game.exe \
	-IC:\SDLLIBS\SDLLIB\SDL2\include \
	-LC:\SDLLIBS\SDLLIB\SDL2\lib \
	-I.\lib\lua \
	-L.\lib\lua \
	-llua53 \
	-lmingw32 \
	-lSDL2main \
	-lSDL2_image \
	-lSDL2_ttf \
	-lSDL2_mixer \
	-lSDL2
clean:
	del game.exe
run:
	game.exe