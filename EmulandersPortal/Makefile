# Emulanders Portal Makefile (ImGui UI)

DEVKITPRO ?= /opt/devkitpro
export DEVKITPRO
export PORTLIBS

export TMP := $(TOPDIR)/build
export TEMP := $(TOPDIR)/build

export PATH := $(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PATH)

TOPDIR ?= $(CURDIR)

TARGET		:= EmulandersPortal
BUILD		:= build
SOURCES		:= source source/imgui source/3rd_party
DATA		:= data
INCLUDES	:= include source/imgui
ICON		:= res/icon.png

# Export app metadata so the sub-make (and switch_rules/nacptool) picks it up properly
export APP_TITLE   := Emulanders Portal
export APP_AUTHOR  := joaoHypo
export APP_VERSION := 1.0.0
export APP_ICON    := $(TOPDIR)/$(ICON)

# Include Switch Rules after defining TARGET, BUILD, APP_TITLE, etc.
include $(DEVKITPRO)/libnx/switch_rules

ARCH		:= -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS		:= -g -Wall -O3 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS		+= $(INCLUDE) -D__SWITCH__ -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -include glad/glad.h -DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS

CXXFLAGS	:= $(CFLAGS) -std=c++17 -O3 -Wno-volatile -fno-rtti -fno-exceptions
ASFLAGS		:= -g $(ARCH)
LDFLAGS		= -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS		:= -lglad -lEGL -lglapi -ldrm_nouveau -lSDL2_image -lSDL2_ttf -lSDL2 -lfreetype -lpng -ljpeg -lwebp -lbz2 -lcurl -lminizip -lmbedtls -lmbedcrypto -lz -lm -lstdc++ -lnx

LIBDIRS		:= $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:= $(CURDIR)/$(TARGET)
export TOPDIR	:= $(CURDIR)

export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			       $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:= $(CC)
else
	export LD	:= $(CXX)
endif

export OFILES_BIN	:= $(addsuffix .o,$(BINFILES))
export OFILES_SRC	:= $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:= $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:= $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			       $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			       -I$(PORTLIBS)/include/SDL2 \
			       -I$(CURDIR)/$(BUILD)

export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export NROFLAGS   := --icon=$(APP_ICON) --nacp=$(CURDIR)/$(TARGET).nacp

.PHONY: $(BUILD) clean all

all: $(BUILD)
	@MSYS2_ARG_CONV_EXCL="-D;$(MSYS2_ARG_CONV_EXCL)" $(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else
.PHONY:	all

DEPENDS	:= $(OFILES:.o=.d)

all	: $(OUTPUT).nro

$(OUTPUT).nro	: $(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf	: $(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

%.bin.o	%_bin.h : %.bin
	@echo $(notdir $(APP_TITLE))
	@$(bin2o)

-include $(DEPENDS)

endif
