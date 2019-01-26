## This file is part of the coreboot project.
##
## Copyright (C) 2017 Facebook Inc.
## Copyright (C) 2018 9elements Cyber Security
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

kernel_tarball=https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-$(CONFIG_LINUXBOOT_KERNEL_VERSION).tar.xz
project_dir=linuxboot
kernel_dir=$(project_dir)/kernel

XGCCPATH?=$(PWD)/util/crossgcc/xgcc/bin
ifeq ($(CONFIG_LINUXBOOT_ARCH),i386)
LINUXBOOT_COMPILE?=$(XGCCPATH)/i386-linux-
ARCH?=x86
else ifeq ($(CONFIG_LINUXBOOT_ARCH),amd64)
LINUXBOOT_COMPILE?=$(XGCCPATH)/x86_64-linux-
ARCH?=x86_64
else ifeq ($(CONFIG_LINUXBOOT_ARCH),arm64)
LINUXBOOT_COMPILE?=$(XGCCPATH)/aarch64-linux-
ARCH?=arm64
endif

OBJCOPY:=$(LINUXBOOT_COMPILE)objcopy

all: kernel

toolchain:
	if [[ ! -x "$(LINUXBOOT_COMPILE)gcc" ]]; then \
	echo "Toolchain '$(LINUXBOOT_COMPILE)*' is missing."; \
	exit 1; \
	fi

$(kernel_dir)/.config:
	echo "    WWW        Download Linux $(CONFIG_LINUXBOOT_KERNEL_VERSION)"
	mkdir -p $(kernel_dir)
ifeq ("$(wildcard $(kernel_dir)/README)","")
	curl -s $(kernel_tarball) | tar xJ -C $(kernel_dir) --strip 1
endif

config: $(kernel_dir)/.config
	echo "    CONFIG     Linux $(CONFIG_LINUXBOOT_KERNEL_VERSION)"
ifneq ($(CONFIG_LINUXBOOT_KERNEL_CONFIGFILE),)
	cp $(CONFIG_LINUXBOOT_KERNEL_CONFIGFILE) $(kernel_dir)/.config
else ifeq ($(CONFIG_LINUXBOOT_ARCH),i386)
	cp x86/defconfig $(kernel_dir)/.config
else ifeq ($(CONFIG_LINUXBOOT_ARCH),amd64)
	cp x86_64/defconfig $(kernel_dir)/.config
else ifeq ($(CONFIG_LINUXBOOT_ARCH),arm64)
	cp arm64/defconfig $(kernel_dir)/.config
endif

ifneq (,$(filter $(CONFIG_LINUXBOOT_ARCH),i386 amd64))
$(kernel_dir)/arch/x86/boot/bzImage: config toolchain
else ifeq ($(CONFIG_LINUXBOOT_ARCH),arm64)
$(kernel_dir)/vmlinux: config toolchain
endif
	echo "    MAKE       Kernel $(CONFIG_LINUXBOOT_KERNEL_VERSION)"
	$(MAKE) -C $(kernel_dir) olddefconfig CROSS_COMPILE=$(LINUXBOOT_COMPILE) ARCH=$(ARCH)
	$(MAKE) -C $(kernel_dir) -j $(CPUS) CROSS_COMPILE=$(LINUXBOOT_COMPILE) ARCH=$(ARCH)

ifneq (,$(filter $(CONFIG_LINUXBOOT_ARCH),i386 amd64))
$(project_dir)/bzImage: $(kernel_dir)/arch/x86/boot/bzImage
	cp $< $@
else ifeq ($(CONFIG_LINUXBOOT_ARCH),arm64)
$(project_dir)/vmlinux.bin: $(kernel_dir)/vmlinux
	$(OBJCOPY) -O binary $< $@

$(project_dir)/target.dtb: $(PWD)/$(CONFIG_LINUXBOOT_DTB_FILE)
	cp $< $@

$(project_dir)/vmlinux.bin.lzma: $(project_dir)/vmlinux.bin
	xz -c -k -f --format=lzma --lzma1=dict=1MiB,lc=3,lp=0,pb=3 $< > $@

$(project_dir)/uImage: $(project_dir)/vmlinux.bin.lzma $(project_dir)/../arm64/kernel_fdt_lzma.its $(project_dir)/target.dtb
	cp $(project_dir)/../arm64/kernel_fdt_lzma.its $(project_dir)
	cp $(PWD)/$(CONFIG_LINUXBOOT_INITRAMFS)$(CONFIG_LINUXBOOT_INITRAMFS_SUFFIX) $(project_dir)/u-initramfs
	mkimage -f $(project_dir)/kernel_fdt_lzma.its $@
endif

ifneq (,$(filter $(CONFIG_LINUXBOOT_ARCH),i386 amd64))
kernel: $(project_dir)/bzImage
else ifeq ($(CONFIG_LINUXBOOT_ARCH),arm64)
kernel: $(project_dir)/uImage
endif

.PHONY: kernel config toolchain
