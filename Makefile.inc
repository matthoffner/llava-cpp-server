#
# Makefile.inc
# Bart Trzynadlowski, 2023
#
# Build rules. Included by Makefile, do not use directly.
#

###############################################################################
# llama.cpp
###############################################################################

include llama.cpp/Makefile

# Take llama files and prepend llama.cpp/ to them
LLAMA_OBJS = $(foreach file,$(OBJS),llama.cpp/$(file))
LLAMA_DEPS = $(foreach file,$(COMMON_DEPS),llama.cpp/$(file))

ifdef LLAMA_CUBLAS
	LLAMA_OBJS += llama.cpp/ggml-cuda.o
endif

###############################################################################
# cpp-httplib
###############################################################################

HTTP_CXXFLAGS = -O2 -std=c++17 -I.. -Wall -Wextra -pthread
HTTP_ZLIB_SUPPORT = -DCPPHTTPLIB_ZLIB_SUPPORT -lz

###############################################################################
# llava-server rules
###############################################################################

bin:
	@if [ ! -d bin/ ]; then \
		echo "Creating directory: bin/"; \
		mkdir bin; \
	fi

obj:
	@if [ ! -d obj/ ]; then \
		echo "Creating directory: obj/"; \
		mkdir obj; \
	fi

obj/%.o:	llama.cpp/examples/llava/%.cpp
	$(CXX) -Illama.cpp -Illama.cpp/common -c -o $@ $< $(CXXFLAGS) -I. -std=c++17

obj/llava_server.o:	llava_server.cpp llava_request.hpp llama.cpp/examples/llava/llava-utils.h llama.cpp/examples/llava/clip.h llama.cpp/common/stb_image.h
	$(CXX) -Illama.cpp -Illama.cpp/common -c -o $@ $< $(CXXFLAGS) -I. -std=c++17

obj/web_server.o: web_server.cpp web_server.hpp llava_request.hpp cpp-httplib/httplib.h
	$(CXX) -c -o $@ $< $(HTTP_CXXFLAGS) -I.

bin/llava-server: obj/llava_server.o obj/web_server.o llama.cpp/ggml.o llama.cpp/llama.o obj/clip.o $(LLAMA_DEPS) $(LLAMA_OBJS)
	$(CXX) -Illama.cpp -Illama.cpp/common $(CXXFLAGS) -o $@ $(LDFLAGS) -Wno-cast-qual $(HTTP_ZLIB_SUPPORT) $(filter-out %.h,$^)

.PHONY: llama-base
llama-base:
	@echo "Building llama.cpp..."
	cd llama.cpp && $(MAKE)

ifdef LLAMA_METAL
bin/ggml-metal.metal: llama.cpp/ggml-metal.metal
	cp $< $@
else
bin/ggml-metal.metal: ;
endif # LLAMA_METAL

ifdef LLAMA_CUBLAS
bin/ggml-cuda.cu: llama.cpp/ggml-cuda.cu
	cp $< $@
else
bin/ggml-cuda.cu: ;
endif # LLAMA_CUBLAS

.PHONY: build-all
build-all:  obj bin llama-base bin/ggml-metal.metal bin/ggml-cuda.cu bin/llava-server
	@echo $(LLAMA_OBJS)

.PHONY: build-clean
build-clean:
	-rm -rf bin/*
	-rm -rf obj/*
	-rmdir bin
	-rmdir obj
	cd llama.cpp && $(MAKE) clean
