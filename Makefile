CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wfatal-errors -g
SDL_PKGS := sdl2 SDL2_image SDL2_ttf SDL2_mixer
SDL_CFLAGS := $(shell pkg-config --cflags $(SDL_PKGS))
SDL_LIBS := $(shell pkg-config --libs $(SDL_PKGS))
TARGET := game

build:
	$(CXX) $(CXXFLAGS) ./src/*.cpp -o $(TARGET) $(SDL_CFLAGS) $(SDL_LIBS)
clean:
	rm -f $(TARGET)
run: build
	./$(TARGET)

.PHONY: build clean run
