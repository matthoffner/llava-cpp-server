#
# Makefile.inc
# Bart Trzynadlowski, 2023
#
# Build rules. Included by Makefile, do not use directly.
#

# CUDA Configuration
ifdef LLAMA_CUBLAS
	MK_CPPFLAGS += -DGGML_USE_CUBLAS -I/usr/local/cuda/include -I/opt/cuda/include -I$(CUDA_PATH)/targets/x86_64-linux/include
	MK_LDFLAGS  += -lcublas -lculibos -lcudart -lcublasLt -lpthread -ldl -lrt -L/usr/local/cuda/lib64 -L/opt/cuda/lib64 -L$(CUDA_PATH)/targets/x86_64-linux/lib
	OBJS        += ggml-cuda.o
	NVCCFLAGS   = --forward-unknown-to-host-compiler -use_fast_math
ifdef LLAMA_CUDA_NVCC
	NVCC = $(LLAMA_CUDA_NVCC)
else
	NVCC = nvcc
endif
endif

# Rule to compile CUDA source
%.o: %.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ $(MK_CPPFLAGS)

###############################################################################
# llama.cpp
###############################################################################

include llama.cpp/Makefile

# Take llama files and prepend llama.cpp/ to them
LLAMA_OBJS = $(foreach file,$(OBJS),llama.cpp/$(file))
LLAMA_DEPS = $(foreach file,$(COMMON_DEPS),llama.cpp/$(file))


###############################################################################
# cpp-httplib
###############################################################################

HTTP_CXXFLAGS = -O2 -std=c++17 -I.. -Wall -Wextra -pthread
HTTP_ZLIB_SUPPORT = -DCPPHTTPLIB_ZLIB_SUPPORT -lz


###############################################################################
# llava-server rules
###############################################################################

#
# bin/ and obj/ directories
#
bin:
	$(info Creating directory: bin/)
	mkdir bin

obj:
	$(info Creating directory: obj/)
	mkdir obj

#
# LLaVa dependencies in llama.cpp/examples/llava/ (e.g., clip.cpp)
#
obj/%.o:	llama.cpp/examples/llava/%.cpp
	$(CXX) -Illama.cpp -Illama.cpp/common -c -o $@ $< $(CXXFLAGS) -I. -std=c++17

#
# Our modules
#
obj/llava_server.o:	llava_server.cpp llava_request.hpp llama.cpp/examples/llava/llava-utils.h llama.cpp/examples/llava/clip.h llama.cpp/common/stb_image.h
	$(CXX) -Illama.cpp -Illama.cpp/common -c -o $@ $< $(CXXFLAGS) -I. -std=c++17

obj/web_server.o: web_server.cpp web_server.hpp llava_request.hpp cpp-httplib/httplib.h
	$(CXX) -c -o $@ $< $(HTTP_CXXFLAGS) -I.

#
# Output binary
# 
bin/llava-server: obj/llava_server.o obj/web_server.o llama.cpp/ggml.o llama.cpp/llama.o obj/clip.o $(LLAMA_DEPS) $(LLAMA_OBJS)
	$(CXX) -Illama.cpp -Illama.cpp/common $(CXXFLAGS) $(MK_LDFLAGS) -o $@ $(LDFLAGS) -Wno-cast-qual $(HTTP_ZLIB_SUPPORT) $(filter-out %.h,$^)

#
# Build llama.cpp
#
.PHONY: llama-base
llama-base:
	$(info Bulding llama.cpp...)
	cd llama.cpp && $(MAKE)

#
# macOS: need to copy ggml-metal.metal to bin/ dir, alongside the binary itself
#
ifdef LLAMA_METAL
bin/ggml-metal.metal: llama.cpp/ggml-metal.metal
	cp $< $@
else
bin/ggml-metal.metal: ;
endif # LLAMA_METAL

#
# Build all
#
build-all:  obj bin llama-base bin/ggml-metal.metal bin/llava-server
	@echo $(LLAMA_OBJS)

#
# Clean all
#
.PHONY: build-clean
build-clean:
	-rm bin/*
	-rm obj/*
	-rmdir bin
	-rmdir obj
	cd llama.cpp && $(MAKE) clean
