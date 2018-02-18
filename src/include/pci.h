/*
 * pci.h
 *
 *  Created on: Nov 16, 2017
 *      Author: jzhu62
 */
#include <types.h>
#ifndef PCI_H_
#define PCI_H_
typedef struct pci_device{
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t header_type;
	uint8_t occupied;
	uint8_t Bus_Num;
	uint8_t Device_Num;
	uint8_t func_num;
	uint8_t PCIBarType;
	uint16_t PCIio_base;
	uint64_t PCImem_base;
}__attribute__((packed))pci_device;

typedef struct pci_ConfigHeader{
	pci_device device[12];
}__attribute__((packed))pci_ConfigHeader;


extern pci_ConfigHeader*  config_pointer;



extern void pci_initialize();


extern pci_device* getPCIDevice(int vendor_id,int device_id );

extern uint8_t getPCIBarType(int bar_number,pci_device* device);

extern uint32_t getPCIbar_io (pci_device* device);

extern uint32_t getPCIBar_mem(pci_device* device);

extern void enablePCIBusMastering(pci_device*device);

#endif /* PCI_H_ */
