/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/io.h>
#include <cbmem.h>
#include <device/pci.h>
#include <stage_cache.h>
#include <cpu/intel/smm/gen1/smi.h>
#include "i945.h"

void stage_cache_external_region(void **base, size_t *size)
{
	/*
	 * The ramstage cache lives in the TSEG region at RESERVED_SMM_OFFSET.
	 * The top of RAM is defined to be the TSEG base address.
	 */
	*size = CONFIG_SMM_RESERVED_SIZE;
	*base = (void *)(northbridge_get_tseg_base()
			 + CONFIG_SMM_RESERVED_SIZE);
}
