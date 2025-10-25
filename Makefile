CXXFLAGS += -Wall -fpic -std=c++23 -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wfloat-equal -Wcast-align -Wundef -Wnon-virtual-dtor -Werror

ifeq ($(OS),Windows_NT)
    CXX = clang++
    TARGET := libfuzzy_sorter.dll
ifeq (,$(findstring $(MSYSTEM),MSYS UCRT64 CLANG64 CLANGARM64 CLANG32 MINGW64 MINGW32))
	# On Windows, but NOT msys/msys2
    MKD = cmd /C mkdir
    RM = cmd /C rmdir /Q /S
else
    MKD = mkdir -p
    RM = rm -rf
endif
else
    ifeq (, $(shell which clang++ 2>/dev/null))
        CXX = g++
				CXXFLAGS += -Wno-stringop-overflow
    else
        CXX = clang++
				CXXFLAGS += -Wno-shorten-64-to-32
    endif
    MKD = mkdir -p
    RM = rm -rf
    TARGET := libfuzzy_sorter.so
endif

all: build/$(TARGET)

build/$(TARGET): src/simple_fuzzy_sorter.cpp src/simple_fuzzy_sorter.h
	$(MKD) build
	$(CXX) -O3 $(CXXFLAGS) -shared src/simple_fuzzy_sorter.cpp -o build/$(TARGET)

# build/test: build/$(TARGET) test/test.c
# 	$(CXX) -Og -ggdb3 $(CFLAGS) test/test.c -o build/test -I./src -L./build -lfzf -lexaminer

.PHONY:
debug: src/simple_fuzzy_sorter.cpp src/simple_fuzzy_sorter.h
	$(MKD) build
	$(CXX) -Og $(CXXFLAGS) -Werror -shared src/simple_fuzzy_sorter.cpp -o build/$(TARGET)

# .PHONY: lint format clangdhappy clean test ntest
# lint:
# 	luacheck lua

# format:
# 	clang-format --style=file --dry-run -Werror src/fzf.c src/fzf.h test/test.c
#
# test: build/test
# 	@LD_LIBRARY_PATH=${PWD}/build:${PWD}/examiner/build:${LD_LIBRARY_PATH} ./build/test
#
# ntest:
# 	nvim --headless --noplugin -u test/minrc.vim -c "PlenaryBustedDirectory test/ { minimal_init = './test/minrc.vim' }"

# langdhappy:
# 	compiledb make

clean:
	$(RM) build
