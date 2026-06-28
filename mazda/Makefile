# Cross-compile Makefile for the CarPlay -> HUD bridge LD_PRELOAD shim.
#
# Builds patches/blmjcicarplay/*.cpp into:
#   build/release/libpatch-blmjcicarplay.so   (default: `make` or `make release`)
#   build/debug/libpatch-blmjcicarplay.so     (`make debug`)
#
# Toolchain: the ARM Cortex-A9 NEON / glibc cross-compiler in the ./m3-toolchain
# git submodule (lmagder/m3-toolchain — an independent, third-party cross-compiler).
# After cloning:  git submodule update --init --recursive
#
# The shim has NO licensing / DRM / network code — it is a plain LD_PRELOAD bridge
# that decodes Apple iAP2 turn-by-turn from the OEM jciCARPLAY process and forwards
# it to the OEM navigation HUD D-Bus API (com.jci.vbs.navi).

M3TOOLCHAIN := ./m3-toolchain
TRIPLE      := arm-cortexa9_neon-linux-gnueabi
CXX         := $(M3TOOLCHAIN)/bin/$(TRIPLE)-g++
SYSROOT     := $(M3TOOLCHAIN)/$(TRIPLE)/sysroot

PATCH := blmjcicarplay

# pkg-config wrapper pointed at the device sysroot (resolves the cross-compiled
# .pc files, not the host's). The CarPlay shim talks to the HUD via the OEM
# libjcidbus transport (patches/blmjcicarplay/oem/), so it does NOT actually link
# dbus-c++; --as-needed below drops the unused dep. The -cflags are still listed
# for parity with the upstream Makefile (harmless if unreferenced).
PKG_CONFIG := PKG_CONFIG_LIBDIR=$(SYSROOT)/usr/lib/pkgconfig \
              PKG_CONFIG_SYSROOT_DIR=$(SYSROOT) pkg-config
PKGS       := dbus-c++-1 dbus-1
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null)
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs   $(PKGS) 2>/dev/null)

# Build-time options (define to taste):
#   CARPLAY_NO_SPLIM      = HUD shows maneuver + distance + road-name only,
#                              never a posted speed-limit (this is the "HUD nav only"
#                              build). Leave defined for the navigation-only release.
#   CARPLAY_VN_NORMALIZE  = strip Vietnamese diacritics so road names render on the
#                              HUD's limited font. Set to 0 / remove for other locales.
FEATURE_FLAGS := -DCARPLAY_NO_SPLIM -DCARPLAY_VN_NORMALIZE=1

# -D_GLIBCXX_USE_CXX11_ABI=0 selects the pre-C++11 (COW) std::string/std::list ABI:
# the device ships libstdc++.so.6.0.14 (GCC 4.6 era) which only knows that ABI.
# -fvisibility=hidden keeps our symbols private; -fPIC is required for a shared object.
COMMON_CXXFLAGS := -std=c++11 \
                   --sysroot=$(SYSROOT) \
                   -march=armv7-a -mtune=cortex-a9 -mfpu=neon \
                   -fPIC \
                   -fvisibility=hidden \
                   -D_GLIBCXX_USE_CXX11_ABI=0 \
                   -Idbus \
                   $(PKG_CFLAGS) \
                   $(FEATURE_FLAGS)
COMMON_LDFLAGS  := --sysroot=$(SYSROOT) -shared \
                   -Wl,--no-undefined \
                   -lpthread -ldl \
                   -Wl,--as-needed $(PKG_LIBS) -Wl,--no-as-needed

RELEASE_CXXFLAGS := $(COMMON_CXXFLAGS) -O3 -DNDEBUG -ffunction-sections -fdata-sections -flto
RELEASE_LDFLAGS  := $(COMMON_LDFLAGS) -O3 -flto -Wl,--gc-sections -Wl,-s
DEBUG_CXXFLAGS   := $(COMMON_CXXFLAGS) -O0 -g3 -DDEBUG
DEBUG_LDFLAGS    := $(COMMON_LDFLAGS)

BUILD_DIR   := build
RELEASE_DIR := $(BUILD_DIR)/release
DEBUG_DIR   := $(BUILD_DIR)/debug

SRCS         := $(shell find patches/$(PATCH) -name '*.cpp' -type f)
RELEASE_OBJS := $(patsubst patches/$(PATCH)/%.cpp,$(RELEASE_DIR)/$(PATCH)/%.o,$(SRCS))
DEBUG_OBJS   := $(patsubst patches/$(PATCH)/%.cpp,$(DEBUG_DIR)/$(PATCH)/%.o,$(SRCS))

.PHONY: all release debug clean
.DEFAULT_GOAL := release

all:     release debug
release: $(RELEASE_DIR)/libpatch-$(PATCH).so
debug:   $(DEBUG_DIR)/libpatch-$(PATCH).so

$(RELEASE_DIR)/$(PATCH)/%.o: patches/$(PATCH)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -MD $(RELEASE_CXXFLAGS) -c $< -o $@

$(DEBUG_DIR)/$(PATCH)/%.o: patches/$(PATCH)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -MD $(DEBUG_CXXFLAGS) -c $< -o $@

$(RELEASE_DIR)/libpatch-$(PATCH).so: $(RELEASE_OBJS)
	$(CXX) $^ $(RELEASE_LDFLAGS) -Wl,-soname,libpatch-$(PATCH).so -o $@
	@echo "built -> $@"

$(DEBUG_DIR)/libpatch-$(PATCH).so: $(DEBUG_OBJS)
	$(CXX) $^ $(DEBUG_LDFLAGS) -Wl,-soname,libpatch-$(PATCH).so -o $@
	@echo "built -> $@"

clean:
	rm -rf $(BUILD_DIR)

-include $(RELEASE_OBJS:.o=.d)
-include $(DEBUG_OBJS:.o=.d)
