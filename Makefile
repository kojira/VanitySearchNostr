#---------------------------------------------------------------------
# Makefile for VanitySearch
#
# Author : Jean-Luc PONS

SRC = Base58.cpp IntGroup.cpp main.cpp Random.cpp \
      Timer.cpp Int.cpp IntMod.cpp Point.cpp SECP256K1.cpp \
      Vanity.cpp NostrOptimized.cpp GPU/GPUGenerate.cpp hash/ripemd160.cpp \
      hash/sha256.cpp hash/sha512.cpp hash/ripemd160_sse.cpp \
      hash/sha256_sse.cpp Bech32.cpp Wildcard.cpp

OBJDIR = obj

# Detect platform and allow ARCH override (e.g. ARCH=x86_64 to build Rosetta binary)
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ARCH ?=
TARGET_ARCH := $(if $(ARCH),$(ARCH),$(UNAME_M))

# Apple Silicon when building for arm64 on macOS
ifeq ($(UNAME_S),Darwin)
ifeq ($(TARGET_ARCH),arm64)
DARWIN_ARM64 := 1
endif
endif

# Add ARM-specific sources
ifeq ($(DARWIN_ARM64),1)
SRC += hash/ripemd160_neon.cpp
endif

# Optional: use system libsecp256k1 via pkg-config (brew install secp256k1)
ifeq ($(USE_LIBSECP256K1),1)
SRC += secp256k1_bridge.cpp
SECP256K1_PKG_CFLAGS := $(shell pkg-config --cflags libsecp256k1 2>/dev/null || pkg-config --cflags secp256k1 2>/dev/null)
SECP256K1_PKG_LIBS   := $(shell pkg-config --libs   libsecp256k1 2>/dev/null || pkg-config --libs   secp256k1 2>/dev/null)
override CXXFLAGS += -DUSE_LIBSECP256K1 $(SECP256K1_PKG_CFLAGS)
override LFLAGS   += $(SECP256K1_PKG_LIBS) -lsecp256k1
endif

# Optional: embed precomputed GTable to eliminate init time
ifeq ($(STATIC_GTABLE),1)
  override CXXFLAGS += -DSTATIC_GTABLE
endif

ifdef gpu

OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o main.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o Vanity.o NostrOptimized.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        $(if $(DARWIN_ARM64),,hash/ripemd160_sse.o) $(if $(DARWIN_ARM64),,hash/sha256_sse.o) \
        $(if $(DARWIN_ARM64),hash/ripemd160_neon.o,) \
        GPU/GPUEngine.o Bech32.o Wildcard.o \
        $(if $(USE_LIBSECP256K1),secp256k1_bridge.o,))

else

OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o main.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o Vanity.o NostrOptimized.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        $(if $(DARWIN_ARM64),,hash/ripemd160_sse.o) $(if $(DARWIN_ARM64),,hash/sha256_sse.o) \
        $(if $(DARWIN_ARM64),hash/ripemd160_neon.o,) \
        Bech32.o Wildcard.o \
        $(if $(USE_LIBSECP256K1),secp256k1_bridge.o,))

endif

CXX        = g++
CUDA       = /usr/local/cuda
CXXCUDA    = /usr/bin/g++
NVCC       = $(CUDA)/bin/nvcc
# nvcc requires joint notation w/o dot, i.e. "5.2" -> "52"
ccap       = $(shell echo $(CCAP) | tr -d '.')

ifeq ($(TARGET_ARCH),arm64)
  # Apple Silicon (CPU only, SSE除外、arm64向け最適化)
  CXX      = clang++
  CXXCUDA  = /usr/bin/clang++
  ifdef gpu
    # Apple SiliconではCUDAは基本的に使用不可。gpu指定時もCPUビルドにフォールバック
    ifdef debug
      CXXFLAGS = -m64 -Wno-write-strings -g -I. -arch arm64
    else
      CXXFLAGS = -m64 -Wno-write-strings -O3 -flto -I. -arch arm64
    endif
    LFLAGS   = -lpthread
  else
    ifdef debug
      CXXFLAGS = -m64 -Wno-write-strings -g -I. -arch arm64
    else
      CXXFLAGS = -m64 -Wno-write-strings -O3 -flto -I. -arch arm64
    endif
    LFLAGS   = -lpthread
  endif
else
  # x86_64 (SSE有効)
  ifeq ($(TARGET_ARCH),x86_64)
    CXX      = clang++
    CXXCUDA  = /usr/bin/clang++
    ifdef gpu
    ifdef debug
    CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -g -I. -I$(CUDA)/include -arch x86_64 -target x86_64-apple-macos10.15
    else
    CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -O2 -I. -I$(CUDA)/include -arch x86_64 -target x86_64-apple-macos10.15
    endif
    LFLAGS     = -lpthread -L$(CUDA)/lib64 -lcudart
    else
    ifdef debug
    CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -g -I. -arch x86_64 -target x86_64-apple-macos10.15
    else
    CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -O2 -I. -arch x86_64 -target x86_64-apple-macos10.15
    endif
    LFLAGS     = -lpthread
    endif
  else
  ifdef gpu
  ifdef debug
  CXXFLAGS   = -DWITHGPU -m64  -mssse3 -Wno-write-strings -g -I. -I$(CUDA)/include
  else
  CXXFLAGS   =  -DWITHGPU -m64 -mssse3 -Wno-write-strings -O2 -I. -I$(CUDA)/include
  endif
  LFLAGS     = -lpthread -L$(CUDA)/lib64 -lcudart
  else
  ifdef debug
  CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -g -I. -I$(CUDA)/include
  else
  CXXFLAGS   =  -m64 -mssse3 -Wno-write-strings -O2 -I. -I$(CUDA)/include
  endif
  LFLAGS     = -lpthread
  endif
  endif
endif


#--------------------------------------------------------------------

ifdef gpu
ifdef debug
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -G -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -g -I$(CUDA)/include -gencode=arch=compute_$(ccap),code=sm_$(ccap) -o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
else
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -O2 -I$(CUDA)/include -gencode=arch=compute_$(ccap),code=sm_$(ccap) -o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
endif
endif

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

all: VanitySearch

VanitySearch: $(OBJET)
	@echo Making VanitySearch...
	$(CXX) $(OBJET) $(LFLAGS) -o VanitySearch

# Ensure object files depend on directory creation
$(OBJET): | $(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash

# Add dependency for source files to force recompilation when switching between gpu/cpu
$(OBJDIR)/main.o: main.cpp
$(OBJDIR)/Vanity.o: Vanity.cpp 
$(OBJDIR)/SECP256K1.o: SECP256K1.cpp
$(OBJDIR)/Bech32.o: Bech32.cpp

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/GPU: $(OBJDIR)
	cd $(OBJDIR) &&	mkdir -p GPU

$(OBJDIR)/hash: $(OBJDIR)
	cd $(OBJDIR) &&	mkdir -p hash

clean:
	@echo Cleaning...
	@rm -f obj/*.o

# Test target for pattern matching
test_pattern: obj/NostrOptimized.o obj/Bech32.o obj/Int.o obj/Point.o obj/SECP256K1.o obj/IntMod.o obj/secp256k1_bridge.o
	@echo "Building pattern matching test..."
	$(CXX) $(CXXFLAGS) -o test_pattern_matching test_pattern_matching.cpp obj/NostrOptimized.o obj/Bech32.o obj/Int.o obj/Point.o obj/SECP256K1.o obj/IntMod.o obj/secp256k1_bridge.o $(LFLAGS) $(SECP256K1_LDFLAGS)
	@rm -f obj/GPU/*.o
	@rm -f obj/hash/*.o
	@rm -f VanitySearch

# Force rebuild when switching between CPU and GPU modes
.PHONY: clean all VanitySearch gpu cpu

# CPU-only build
cpu:
	$(MAKE) clean
	$(MAKE) VanitySearch

# GPU build  
gpu:
	$(MAKE) clean
	$(MAKE) VanitySearch gpu=1

# Rosetta (x86_64) CPU-only build on Apple Silicon for baseline
cpu_x86_64:
	$(MAKE) clean
	$(MAKE) VanitySearch ARCH=x86_64

