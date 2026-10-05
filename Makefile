CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wfatal-errors -g
PKGS := sdl2 SDL2_image SDL2_ttf SDL2_mixer lua
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS := $(shell pkg-config --libs $(PKGS))
TARGET := game
SRCS := $(wildcard src/*.cpp src/*/*.cpp)
TEST_TARGET := run_tests
TEST_SRCS := $(wildcard tests/*.cpp) $(filter-out src/Main.cpp,$(SRCS))

build:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(PKG_CFLAGS) $(PKG_LIBS)
# AddressSanitizer can't be used (Homebrew's SDL2 aborts when loaded under it),
# so libc++ hardening stands in for it by trapping out-of-bounds container access.
TEST_FLAGS := -fsanitize=undefined -fno-sanitize-recover=undefined \
	-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
test:
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) -Isrc $(TEST_SRCS) -o $(TEST_TARGET) $(PKG_CFLAGS) $(PKG_LIBS)
	./$(TEST_TARGET)
clean:
	rm -f $(TARGET) $(TEST_TARGET)
run: build
	./$(TARGET)

.PHONY: build clean run test
