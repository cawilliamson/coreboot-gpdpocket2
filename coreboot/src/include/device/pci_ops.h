#ifndef PCI_OPS_H
#define PCI_OPS_H

#include <stdint.h>
#include <device/device.h>
#include <arch/pci_ops.h>

#ifndef __SIMPLE_DEVICE__
u8 pci_read_config8(struct device *dev, unsigned int where);
u16 pci_read_config16(struct device *dev, unsigned int where);
u32 pci_read_config32(struct device *dev, unsigned int where);
void pci_write_config8(struct device *dev, unsigned int where, u8 val);
void pci_write_config16(struct device *dev, unsigned int where, u16 val);
void pci_write_config32(struct device *dev, unsigned int where, u32 val);

#endif

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_or_config8(pci_devfn_t dev, unsigned int where, u8 ormask)
#else
static __always_inline
void pci_or_config8(struct device *dev, unsigned int where, u8 ormask)
#endif
{
	u8 value = pci_read_config8(dev, where);
	pci_write_config8(dev, where, value | ormask);
}

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_or_config16(pci_devfn_t dev, unsigned int where, u16 ormask)
#else
static __always_inline
void pci_or_config16(struct device *dev, unsigned int where, u16 ormask)
#endif
{
	u16 value = pci_read_config16(dev, where);
	pci_write_config16(dev, where, value | ormask);
}

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_or_config32(pci_devfn_t dev, unsigned int where, u32 ormask)
#else
static __always_inline
void pci_or_config32(struct device *dev, unsigned int where, u32 ormask)
#endif
{
	u32 value = pci_read_config32(dev, where);
	pci_write_config32(dev, where, value | ormask);
}

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_update_config8(pci_devfn_t dev, int reg, u8 mask, u8 or)
#else
static __always_inline
void pci_update_config8(struct device *dev, int reg, u8 mask, u8 or)
#endif
{
	u8 reg8;

	reg8 = pci_read_config8(dev, reg);
	reg8 &= mask;
	reg8 |= or;
	pci_write_config8(dev, reg, reg8);
}

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_update_config16(pci_devfn_t dev, int reg, u16 mask, u16 or)
#else
static __always_inline
void pci_update_config16(struct device *dev, int reg, u16 mask, u16 or)
#endif
{
	u16 reg16;

	reg16 = pci_read_config16(dev, reg);
	reg16 &= mask;
	reg16 |= or;
	pci_write_config16(dev, reg, reg16);
}

#ifdef __SIMPLE_DEVICE__
static __always_inline
void pci_update_config32(pci_devfn_t dev, int reg, u32 mask, u32 or)
#else
static __always_inline
void pci_update_config32(struct device *dev, int reg, u32 mask, u32 or)
#endif
{
	u32 reg32;

	reg32 = pci_read_config32(dev, reg);
	reg32 &= mask;
	reg32 |= or;
	pci_write_config32(dev, reg, reg32);
}

const struct pci_bus_operations *pci_bus_default_ops(struct device *dev);

#endif /* PCI_OPS_H */
