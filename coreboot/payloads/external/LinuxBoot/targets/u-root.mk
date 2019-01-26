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

project_dir=$(shell pwd)/linuxboot
go_path_dir=$(project_dir)/go
uroot_bin=$(project_dir)/u-root
uroot_package=github.com/u-root/u-root

go_version=$(shell go version | sed -nr 's/.*go([0-9]+\.[0-9]+.?[0-9]?).*/\1/p' )
go_version_major=$(shell echo $(go_version) |  sed -nr 's/^([0-9]+)\.([0-9]+)\.?([0-9]*)$$/\1/p')
go_version_minor=$(shell echo $(go_version) |  sed -nr 's/^([0-9]+)\.([0-9]+)\.?([0-9]*)$$/\2/p')

uroot_args+=-build=$(CONFIG_LINUXBOOT_UROOT_FORMAT)
uroot_args+=-initcmd $(CONFIG_LINUXBOOT_UROOT_INITCMD)
uroot_args+=-defaultsh $(CONFIG_LINUXBOOT_UROOT_SHELL)
ifneq (CONFIG_LINUXBOOT_UROOT_FILES,)
uroot_args+=$(foreach file,$(CONFIG_LINUXBOOT_UROOT_FILES),-files $(PWD)/$(file))
endif

uroot_cmds=$(CONFIG_LINUXBOOT_UROOT_COMMANDS)

all: u-root

version:
ifeq ("$(go_version)","")
	printf "\n<<Please install Golang >= 1.9 for u-root mode>>\n\n"
	exit 1
endif
ifeq ($(shell if [ $(go_version_major) -eq 1 ]; then echo y; fi),y)
ifeq ($(shell if [ $(go_version_minor) -lt 9 ]; then echo y; fi),y)
	printf "\n  Golang version $(go_version) currently installed.\n\
	<<Please install Golang >= 1.9 for u-root mode>>\n\n"
	exit 1
endif
endif

get: version
	if [ -d "$(go_path_dir)/src/$(uroot_package)" ]; then \
	git -C $(go_path_dir)/src/$(uroot_package) checkout --quiet master; \
	GOPATH=$(go_path_dir) go get -d -u -v $(uroot_package) || \
	echo -e "\n<<u-root package update failed>>\n"; \
	else \
	GOPATH=$(go_path_dir) go get -d -u -v $(uroot_package) || \
	(echo -e "\n<<failed to get u-root package. Please check your internet access>>\n" && \
	exit 1); \
	fi

checkout: get
	git -C $(go_path_dir)/src/$(uroot_package) checkout --quiet $(CONFIG_LINUXBOOT_UROOT_VERSION)

build: checkout
	GOPATH=$(go_path_dir) go build -o $(uroot_bin) $(uroot_package)

u-root: build
	GOARCH=$(CONFIG_LINUXBOOT_ARCH) GOPATH=$(go_path_dir) $(uroot_bin) \
	$(uroot_args) -o $(project_dir)/initramfs_u-root.cpio $(uroot_cmds)

.PHONY: all u-root build checkout get version
