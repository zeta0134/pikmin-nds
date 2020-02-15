#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	pikmin-nds
export TOPDIR		:=	$(CURDIR)

NITRODATA	:=	arm9/nitrofs

ifneq ($(strip $(NITRODATA)),)
	export NITRO_FILES	:=	$(CURDIR)/$(NITRODATA)
endif

.PHONY: arm7/$(TARGET).elf arm9/$(TARGET).elf arm9/$(TARGET)_test.elf clean clean-nitrofs clean-actors

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(TARGET).nds
test: $(TARGET)_test.nds

#---------------------------------------------------------------------------------
$(TARGET).nds	:	arm7/$(TARGET).elf arm9/$(TARGET).elf
	$(DEVKITARM)/bin/ndstool	-c $(TARGET).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf -d $(NITRO_FILES)

#---------------------------------------------------------------------------------
$(TARGET)_test.nds	:	arm7/$(TARGET).elf arm9/$(TARGET)_test.elf
	$(DEVKITARM)/bin/ndstool	-c $(TARGET)_test.nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET)_test.elf -d $(NITRO_FILES)

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

arm9/$(TARGET)_test.elf:
	TARGET=$(TARGET)_test $(MAKE) -C arm9 test


#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET).nds $(TARGET)_test.nds $(TARGET).arm7 $(TARGET).arm9

clean-nitrofs:
	$(MAKE) -C arm9 clean-nitrofs

clean-actors:
	$(MAKE) -C arm9 clean-actors
