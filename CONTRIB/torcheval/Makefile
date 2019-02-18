# Compilation parameters
CPP     = g++
CPPFLAGS = -O3 -Wall -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0
CPPINCLUDES = -Ilibtorch/include -Ilibtorch/include/torch/csrc/api/include

include ../../Makefile.vars

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

torcheval.o: torcheval.c torcheval.h 
	$(CC) $(CFLAGS) -I../../include -o $@ -c $<

.PHONY: clean
clean: 
	rm -rf *.o *.a
	@echo "Directory cleaned."

