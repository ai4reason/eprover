# Compilation parameters
CPP     = g++
CPPFLAGS = -O3 -Wall -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0
CPPINCLUDES = -Ilibtorch/include -Ilibtorch/include/torch/csrc/api/include

CC        = gcc
CFLAGS    = -O3 -Wall
CINCLUDES = -I../../include

# Default rule is to build the executable.
all: torcheval.a

# Create an object archive (will be statically linked).
torcheval.a: torcheval.o torch.o
	@rm -f $@
	ar rcs $@ torcheval.o torch.o
	ranlib $@

# Pattern rule to compile all the CPP files.
%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(CPPINCLUDES) -o $@ -c $<

# Pattern rule to compile all the C files.
%.o: %.c
	$(CC) $(CFLAGS) $(CINCLUDES) -o $@ -c $<

.PHONY: clean
clean: 
	rm -rf *.o *.a
	@echo "Directory cleaned."

