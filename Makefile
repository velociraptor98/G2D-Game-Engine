CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wfatal-errors -g -MMD -MP
PKGS := sdl2 SDL2_image SDL2_ttf SDL2_mixer lua
PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS := $(shell pkg-config --libs $(PKGS))
BUILD := build

ENGINE_SRCS := $(shell find engine -name '*.cpp')
APP_SRCS := $(wildcard apps/g2d/*.cpp)
EXTENSION_SRCS := $(wildcard examples/cpp-extension/*.cpp)
TEST_SRCS := $(wildcard tests/*.cpp)

ENGINE_LIB := $(BUILD)/libg2d.a
RUNNER := g2d
EXTENSION_DEMO := extension_demo
TEST_RUNNER := run_tests

# AddressSanitizer can't be used (Homebrew's SDL2 aborts when loaded under it),
# so libc++ hardening stands in for it by trapping out-of-bounds container access.
# Tests get their own build of the engine so these flags apply to it too.
TEST_FLAGS := -fsanitize=undefined -fno-sanitize-recover=undefined \
	-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG

build: $(RUNNER) $(EXTENSION_DEMO)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iengine $(PKG_CFLAGS) -c $< -o $@

$(BUILD)/test/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) -Iengine $(PKG_CFLAGS) -c $< -o $@

# Recreated rather than updated, so objects of deleted sources don't linger in it.
$(ENGINE_LIB): $(ENGINE_SRCS:%.cpp=$(BUILD)/%.o)
	rm -f $@
	ar rcs $@ $^

$(RUNNER): $(APP_SRCS:%.cpp=$(BUILD)/%.o) $(ENGINE_LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(PKG_LIBS)

$(EXTENSION_DEMO): $(EXTENSION_SRCS:%.cpp=$(BUILD)/%.o) $(ENGINE_LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(PKG_LIBS)

$(TEST_RUNNER): $(TEST_SRCS:%.cpp=$(BUILD)/test/%.o) $(ENGINE_SRCS:%.cpp=$(BUILD)/test/%.o)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $^ -o $@ $(PKG_LIBS)

test: $(TEST_RUNNER)
	./$(TEST_RUNNER)

run: $(RUNNER)
	./$(RUNNER)

jungle: $(RUNNER)
	./$(RUNNER) games/jungle/level1.lua

clean:
	rm -rf $(BUILD) $(RUNNER) $(EXTENSION_DEMO) $(TEST_RUNNER) *.dSYM

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: build test run jungle clean
