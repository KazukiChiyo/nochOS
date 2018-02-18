/*
 * pci.c
 *
 *  Created on: Nov 16, 2017
 *      Author: jzhu62
 *      citation: http://wiki.osdev.org/PCI#Multifunction_Devices
 */
#include <pci.h>
#include <lib.h>
#include <types.h>
#include <ports.h>
static pci_ConfigHeader config;
pci_ConfigHeader*  config_pointer = &config;




pci_device* getPCIDevice(int vendor_id,int device_id ){
  int i = 0 ;
  while (config.device[i].vendor_id!=vendor_id||config.device[i].device_id!=device_id){
	  i++;
  }
  return &(config.device[i]);
}

uint8_t getPCIBarType(int bar_number,pci_device* device){
	return device->PCIBarType;
}

uint32_t getPCIbar_io (pci_device* device){
	return device->PCIio_base;
}

uint32_t getPCIBar_mem(pci_device* device){
	return device->PCImem_base;
}

void enablePCIBusMastering(pci_device*device){
	   uint32_t address;
	   uint32_t lbus  = (uint32_t)device->Bus_Num;
	   uint32_t lslot = (uint32_t)device->Device_Num;
	   uint32_t lfunc = (uint32_t)device->func_num;
	   uint8_t offset = 4;
	   uint32_t tmp;
	   address = (uint32_t)((lbus << 16) | (lslot << 11) |
	             (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	   /* write out the address */
	   outportl (0xCF8, address);
	   /* read in the data */
	   /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
	   tmp = inportl (0xCFC) ;
	   outportl (0xCFC,tmp|0x00000004); // enable master
	   tmp = inportl (0xCFC) ;
}
uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                            uint8_t func, uint8_t offset)
{
   uint32_t address;
   uint32_t lbus  = (uint32_t)bus;
   uint32_t lslot = (uint32_t)slot;
   uint32_t lfunc = (uint32_t)func;
   uint16_t tmp = 0;

   /* create configuration address as per Figure 1 */
   address = (uint32_t)((lbus << 16) | (lslot << 11) |
             (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

   /* write out the address */
   outportl (0xCF8, address);
   /* read in the data */
   /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
   tmp = (uint16_t)((inportl (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
   return (tmp);
}
uint32_t pciConfigReadWord_32 (uint8_t bus, uint8_t slot,
                            uint8_t func, uint8_t offset)
{
   uint32_t address;
   uint32_t lbus  = (uint32_t)bus;
   uint32_t lslot = (uint32_t)slot;
   uint32_t lfunc = (uint32_t)func;
   uint32_t tmp = 0;

   /* create configuration address as per Figure 1 */
   address = (uint32_t)((lbus << 16) | (lslot << 11) |
             (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

   /* write out the address */
   outportl (0xCF8, address);
   /* read in the data */
   /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
   tmp = inportl (0xCFC);
   return (tmp);
}

uint16_t getVendorID(bus, device, function){
	return pciConfigReadWord(bus,device,function,0);
}
uint8_t getHeaderType(uint8_t bus, uint8_t slot, uint8_t function){
	return pciConfigReadWord(bus,slot,function,0xE)&0x7f;
}
//void checkFunction(uint8_t bus, uint8_t slot, uint8_t function){
//
//}
void adddevice(uint8_t bus, uint8_t slot, uint8_t function){
	int i=0;
	for (i=0;i<12;i++){
		if(config.device[i].occupied==0){
			config.device[i].occupied=1;
			config.device[i].Bus_Num=bus;
			config.device[i].Device_Num=slot;
			config.device[i].func_num=function;
			config.device[i].vendor_id=pciConfigReadWord(bus,slot,function,0);
			config.device[i].device_id=pciConfigReadWord(bus,slot,function,0x2);
			config.device[i]. header_type = getHeaderType(bus, slot, function);
			//didn't consider header type 2
			config.device[i].PCIBarType= pciConfigReadWord(bus,slot,function,0x10)&0x1;
			config.device[i].PCIio_base= pciConfigReadWord_32(bus,slot,function,0x10)&0xFFFFFFFC;
			config.device[i].PCImem_base= pciConfigReadWord_32(bus,slot,function,0x10)&0xFFFFFFF0;
			break;
		}
	}
}
void checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;

    uint16_t vendorID = getVendorID(bus, device, function);
    if(vendorID == 0xFFFF) return;        // Device doesn't exist
    adddevice(bus,device,function);
   // checkFunction(bus, device, function);
    uint8_t headerType = getHeaderType(bus, device, function);
    if( (headerType & 0x80) != 0) {
        /* It is a multi-function device, so check remaining functions */
        for(function = 1; function < 8; function++) {
            if(getVendorID(bus, device, function) != 0xFFFF) {
                //checkFunction(bus, device, function);
                adddevice(bus,device,function);
            }
        }
    }
}
void checkAllBuses() {
    uint16_t bus;
    uint8_t device;

    for(bus = 0; bus < 256; bus++) {
        for(device = 0; device < 32; device++) {
            checkDevice(bus, device);
        }
    }
}
void pci_initialize(){
	int i = 0 ;
	for (i=0;i<12;i++){
		(config.device[i]).occupied=0;
	}
	checkAllBuses();

}
