/*
 * network.c
 *
 *  Created on: Nov 16, 2017
 *      Author: jzhu62
 *      citation :http://wiki.osdev.org/Intel_Ethernet_i217
 */
#include <network.h>
#include <types.h>
#include <ports.h>
#include <pci.h>
#include <lib.h>
#include <paging.h>
#include <i8259.h>
#include <interrupt.h>
#include <debug.h>
#include <mem.h>
#include <time.h>


// inspire by http://wiki.osdev.org/Intel_Ethernet_i217;
static uint8_t e1000_rx_desc_arr [sizeof(struct e1000_rx_desc)*E1000_NUM_RX_DESC+16];
static uint8_t e1000_tx_desc_arr [sizeof(struct e1000_tx_desc)*E1000_NUM_TX_DESC+16];
static uint8_t e1000_rx_temp [8192+16];
static dhcp c;
static NetworkDriver driver;
static ipconfig ipconf;
NetworkDriver* pointer = &driver;

static uint8_t * package_buf;
static uint32_t package_len = 0;
static uint32_t ip [4];
static volatile uint32_t SEND_DHCP = 0;

//MMIOutils struct function

uint16_t switch_endian16(uint16_t nb){
	return((nb>>8)|(nb<<8));
}
uint32_t switch_endian32(uint32_t nb){
	return(((nb>>24)&0xff)|
			((nb<<8)&0xff0000)|
			((nb>>8)&0xff00)|
			((nb<<24)&0xff000000));
}
uint8_t read8 (uint32_t p_address)
{
    return *((volatile uint8_t*)(p_address));
}
uint16_t read16 (uint32_t p_address)
{
    return *((volatile uint16_t*)(p_address));

}
uint32_t read32 (uint32_t p_address)
{
    return *((volatile uint32_t*)(p_address));

}
uint64_t read64 (uint32_t p_address)
{
    return *((volatile uint64_t*)(p_address));
}
void write8 (uint32_t p_address,uint8_t p_value)
{
    (*((volatile uint8_t*)(p_address)))=(p_value);
}
void write16 (uint32_t p_address,uint16_t p_value)
{
    (*((volatile uint16_t*)(p_address)))=(p_value);
}
void write32 (uint32_t p_address,uint32_t p_value)
{
    (*((volatile uint32_t*)(p_address)))=(p_value);

}
void write64 (uint32_t p_address,uint64_t p_value)
{
    (*((volatile uint64_t*)(p_address)))=(p_value);
}
void send_dhcp_discover(){
	//printf("something went wrong");
	c.a[0]=switch_endian32(0xffffffff);
	c.a[1]=switch_endian32(0xffff5254);
	c.a[2]=switch_endian32(0x00123456);
	c.a[3]=switch_endian32(0x08004510);
	c.a[4]=switch_endian32(0x01480000);
	c.a[5]=switch_endian32(0x00001011);
	c.a[6]=switch_endian32(0xa9960000);
	c.a[7]=switch_endian32(0x0000ffff);
	c.a[8]=switch_endian32(0xffff0044);
	c.a[9]=switch_endian32(0x00430134);
	c.a[10]=switch_endian32(0x1c6d0101);
	c.a[11]=switch_endian32(0x0600fa5f);
	c.a[12]=switch_endian32(0xc0130008);
	c.a[13]=switch_endian32(0x00000000);
	c.a[14]=switch_endian32(0x00000000);
	c.a[15]=switch_endian32(0x00000000);
	c.a[16]=switch_endian32(0x00000000);
	c.a[17]=switch_endian32(0x00005254);
	c.a[18]=switch_endian32(0x00123456);
	c.a[19]=switch_endian32(0x00000000);
	c.a[20]=switch_endian32(0x00000000);
	c.a[21]=switch_endian32(0x00000000);
	c.a[22]=switch_endian32(0x00000000);
	c.a[23]=switch_endian32(0x00000000);
	c.a[24]=switch_endian32(0x00000000);
	c.a[25]=switch_endian32(0x00000000);
	c.a[26]=switch_endian32(0x00000000);
	c.a[27]=switch_endian32(0x00000000);
	c.a[28]=switch_endian32(0x00000000);
	c.a[29]=switch_endian32(0x00000000);
	c.a[30]=switch_endian32(0x00000000);
	c.a[31]=switch_endian32(0x00000000);
	c.a[32]=switch_endian32(0x00000000);
	c.a[33]=switch_endian32(0x00000000);
	c.a[34]=switch_endian32(0x00000000);
	c.a[35]=switch_endian32(0x00000000);
	c.a[36]=switch_endian32(0x00000000);
	c.a[37]=switch_endian32(0x00000000);
	c.a[38]=switch_endian32(0x00000000);
	c.a[39]=switch_endian32(0x00000000);
	c.a[40]=switch_endian32(0x00000000);
	c.a[41]=switch_endian32(0x00000000);
	c.a[42]=switch_endian32(0x00000000);
	c.a[43]=switch_endian32(0x00000000);
	c.a[44]=switch_endian32(0x00000000);
	c.a[45]=switch_endian32(0x00000000);
	c.a[46]=switch_endian32(0x00000000);
	c.a[47]=switch_endian32(0x00000000);
	c.a[48]=switch_endian32(0x00000000);
	c.a[49]=switch_endian32(0x00000000);
	c.a[50]=switch_endian32(0x00000000);
	c.a[51]=switch_endian32(0x00000000);
	c.a[52]=switch_endian32(0x00000000);
	c.a[53]=switch_endian32(0x00000000);
	c.a[54]=switch_endian32(0x00000000);
	c.a[55]=switch_endian32(0x00000000);
	c.a[56]=switch_endian32(0x00000000);
	c.a[57]=switch_endian32(0x00000000);
	c.a[58]=switch_endian32(0x00000000);
	c.a[59]=switch_endian32(0x00000000);
	c.a[60]=switch_endian32(0x00000000);
	c.a[61]=switch_endian32(0x00000000);
	c.a[62]=switch_endian32(0x00000000);
	c.a[63]=switch_endian32(0x00000000);
	c.a[64]=switch_endian32(0x00000000);
	c.a[65]=switch_endian32(0x00000000);
	c.a[66]=switch_endian32(0x00000000);
	c.a[67]=switch_endian32(0x00000000);
	c.a[68]=switch_endian32(0x00000000);
	c.a[69]=switch_endian32(0x00006382);
	c.a[70]=switch_endian32(0x53633501);
	c.a[71]=switch_endian32(0x0132040a);
	c.a[72]=switch_endian32(0xc1b76337);
	c.a[73]=switch_endian32(0x0a011c02);
	c.a[74]=switch_endian32(0x030f060c);
	c.a[75]=switch_endian32(0x28292aff);
	c.a[76]=switch_endian32(0x00000000);
	c.a[77]=switch_endian32(0x00000000);
	c.a[78]=switch_endian32(0x00000000);
	c.a[79]=switch_endian32(0x00000000);
	c.a[80]=switch_endian32(0x00000000);
	c.a[81]=switch_endian32(0x00000000);
	c.a[82]=switch_endian32(0x00000000);
	c.a[83]=switch_endian32(0x00000000);
	c.a[84]=switch_endian32(0x00000000);
	c.b=0;
	//a[85]=switch_endian32(0x0000);
	sendPacket(&c,sizeof(c));
	//printf("dhcp package send complete\n");
}
//network function declearation

int32_t get_package(void * buf, uint32_t nbytes) {
    if (package_len == 0)
        return -1;

    if (package_len < nbytes)
        nbytes = package_len;

    memcpy(buf, package_buf, nbytes);
    //send_dhcp_discover();
    package_len = 0;
    return nbytes;
}
int32_t get_ip(){
	SEND_DHCP=0;
	send_dhcp_discover();
	sti();
	uint32_t ms = system_time.count_ms;
	while (SEND_DHCP == 0) {
		if (system_time.count_ms - ms > 1000)
			return -1;
	}
	return 0;
}
int32_t get_ipconfig(void*buf){
	int i =0;
	for (i =0 ; i<6; i++){
		ipconf.mac[i]=pointer->mac[i];
	}
	for (i = 0 ; i<4;i++){
		ipconf.ip_address[i]=ip[i];
	}
    uint32_t ipconfig_value_TCTRL=readCommand(REG_TCTRL);
    uint32_t ipconfig_value_RCTRL = readCommand(REG_RCTRL);
    ipconf.RCTL__BAM = (ipconfig_value_RCTRL& (RCTL_BAM))>>15;
    ipconf.RCTL__CFIEN = (ipconfig_value_TCTRL& (RCTL_CFIEN))>>3;
    ipconf.RCTL__DPF = (ipconfig_value_RCTRL& (RCTL_DPF))>>22;
    ipconf.RCTL__EN = (ipconfig_value_RCTRL& (RCTL_EN))>>1;
    ipconf.RCTL__LBM_NONE = (ipconfig_value_RCTRL& (RCTL_LBM_NONE))>>6;
    ipconf.RCTL__LPE = (ipconfig_value_RCTRL& (RCTL_LPE))>>5;
    ipconf.RCTL__MPE = (ipconfig_value_RCTRL& (RCTL_MPE))>>4;
    ipconf.RCTL__PMCF = (ipconfig_value_RCTRL& (RCTL_PMCF))>>23;
    ipconf.RCTL__SBP = (ipconfig_value_RCTRL& (RCTL_SBP))>>2;
    ipconf.RCTL__SECRC = (ipconfig_value_RCTRL& (RCTL_SECRC))>>26;
    ipconf.RCTL__UPE = (ipconfig_value_RCTRL& (RCTL_UPE))>>3;
    ipconf.RCTL__VFE = (ipconfig_value_RCTRL& (RCTL_VFE))>>18;
    ipconf.TCTL__EN = (ipconfig_value_TCTRL& (TCTL_EN))>>1;
    ipconf.TCTL__PSP = (ipconfig_value_TCTRL& (TCTL_PSP))>>3;
    ipconf.TCTL__RTLC = (ipconfig_value_TCTRL& (TCTL_RTLC))>>24;
    ipconf.TCTL__SWXOFF = (ipconfig_value_TCTRL& (TCTL_SWXOFF))>>22;
    memcpy(buf, &ipconf,24 );


    return 0;
}

void writeCommand( uint16_t p_address, uint32_t p_value)
{
    if (pointer->bar_type == 0 )
    {
        write32(pointer->mem_base+p_address,p_value);
    }
    else
    {
        outportl(pointer->io_base, p_address);
        outportl(pointer->io_base + 4, p_value);
    }
}
uint32_t readCommand( uint16_t p_address)
{
    if (pointer-> bar_type == 0 )
    {
        return read32(pointer->mem_base+p_address);
    }
    else
    {
        outportl(pointer->io_base, p_address);
        return inportl(pointer->io_base + 4);
    }
}
uint32_t detectEEProm()
{
	int i;
    uint32_t val = 0;
    writeCommand(REG_EEPROM, 0x1);

    for(i = 0; i < 1000 && ! pointer->eerprom_exists; i++)
    {
            val = readCommand( REG_EEPROM);
            if(val & 0x10)
            	pointer->eerprom_exists = TRUE;
            else
            	pointer->eerprom_exists = FALSE;
    }
    return pointer->eerprom_exists;
}

uint32_t eepromRead( uint8_t addr)
{
	uint16_t data = 0;
	uint32_t tmp = 0;
        if ( pointer->eerprom_exists)
        {
            	writeCommand( REG_EEPROM, (1) | ((uint32_t)(addr) << 8) );
        	while( !((tmp = readCommand(REG_EEPROM)) & (1 << 4)) );
        }
        else
        {
            writeCommand( REG_EEPROM, (1) | ((uint32_t)(addr) << 2) );
            while( !((tmp = readCommand(REG_EEPROM)) & (1 << 1)) );
        }
	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}
uint32_t readMACAddress()
{
	int i;
    if ( pointer->eerprom_exists)
    {
        uint32_t temp;
        temp = eepromRead( 0);
        pointer->mac[0] = temp &0xff;
        pointer->mac[1] = temp >> 8;
        temp = eepromRead( 1);
        pointer->mac[2] = temp &0xff;
        pointer->mac[3] = temp >> 8;
        temp = eepromRead( 2);
        pointer->mac[4] = temp &0xff;
        pointer->mac[5] = temp >> 8;
    }
    else
    {
        uint8_t * mem_base_mac_8 = (uint8_t *) (pointer->mem_base+0x5400);
        uint32_t * mem_base_mac_32 = (uint32_t *) (pointer->mem_base+0x5400);
        if ( mem_base_mac_32[0] != 0 )
        {
            for( i = 0; i < 6; i++)
            {
            	pointer->mac[i] = mem_base_mac_8[i];
            }
        }
        else return FALSE;
    }
    return TRUE;
}
void rxinit()
{
	int i;
   // uint8_t * ptr;
    struct e1000_rx_desc *descs;

    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones


    descs = (struct e1000_rx_desc *)e1000_rx_desc_arr;
    for(i = 0; i < E1000_NUM_RX_DESC; i++)
    {
    	pointer->rx_descs[i] = (struct e1000_rx_desc *)((uint8_t *)descs + i*16);
    	pointer->rx_descs[i]->addr = (uint64_t)(uint8_t *)e1000_rx_temp;
    	pointer->rx_descs[i]->status = 0;
    }

    writeCommand(REG_TXDESCLO, (uint32_t)((uint64_t)e1000_rx_desc_arr >> 32) );
    writeCommand(REG_TXDESCHI, (uint32_t)((uint64_t)e1000_rx_desc_arr & 0xFFFFFFFF));

    writeCommand(REG_RXDESCLO, (uint64_t)e1000_rx_desc_arr);
    writeCommand(REG_RXDESCHI, 0);

    writeCommand(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    writeCommand(REG_RXDESCHEAD, 0);
    writeCommand(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
    pointer->rx_cur = 0;
    writeCommand(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);

}


void txinit()
{
	int i ;
    struct e1000_tx_desc *descs;
    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones

    descs = (struct e1000_tx_desc *)e1000_tx_desc_arr;
    for(i = 0; i < E1000_NUM_TX_DESC; i++)
    {
    	pointer->tx_descs[i] = (struct e1000_tx_desc *)((uint8_t*)descs + i*16);
    	pointer->tx_descs[i]->addr = 0;
    	pointer->tx_descs[i]->cmd = 0;
    	pointer->tx_descs[i]->status = TSTA_DD;
    }

    writeCommand(REG_TXDESCHI, (uint32_t)((uint64_t)e1000_tx_desc_arr >> 32) );
    writeCommand(REG_TXDESCLO, (uint32_t)((uint64_t)e1000_tx_desc_arr & 0xFFFFFFFF));


    //now setup total length of descriptors
    writeCommand(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);


    //setup numbers
    writeCommand( REG_TXDESCHEAD, 0);
    writeCommand( REG_TXDESCTAIL, 0);
    pointer->tx_cur = 0;
    writeCommand(REG_TCTRL,  TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC);

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards
    // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    writeCommand(REG_TCTRL, 0x3003F0FA); //0b0110000000000111111000011111010);
    writeCommand(REG_TIPG,  0x0060200A);

//    printf("the read command is %x",readCommand(REG_TCTRL));
//    while(1);

}
void enableInterrupt()
{
    writeCommand(REG_IMASK ,0x1F6DC);
    writeCommand(REG_IMASK ,0xff & ~4);
    readCommand(0xc0);

}
void E1000(pci_device * pci_device)
{
    // Get BAR0 type, io_base address and MMIO base address
	pointer->bar_type = getPCIBarType(0,pci_device);
	pointer->io_base = getPCIbar_io(pci_device);// & ~1;
	pointer->mem_base = getPCIBar_mem(pci_device);// & ~3;

    // Off course you will need here to map the memory address into you page tables and use corresponding virtual addresses

    // Enable bus mastering
    enablePCIBusMastering(pci_device);
    pointer->eerprom_exists = FALSE;
}
void printMac(){
	printf ("MAC address is:");
	int i;
	for(i=0;i<6;i++){
		printf("%x ",pointer->mac[i]);
	}
	printf("\n");
}
uint32_t net_start ()
{
	int i;
	package_buf = (uint8_t *)malloc(1024);
    detectEEProm (pointer);
    if (! readMACAddress()) return FALSE;
    //startLink();  ////////startlink

    for(i = 0; i < 0x80; i++)
        writeCommand(0x5200 + i*4, 0);
    enable_irq(NET_WORK_PIN);
    int_set_idt(NET_WORK_PIN);
    enableInterrupt();
#ifdef RUN_TESTS
    printMac();
    printf("enable network interrupt");
#endif
    rxinit();
    txinit();
#ifdef RUN_TESTS
    printf("E1000 card started\n");
#endif
    //while(1);
    return 1;
}
void fire ()
{
		//printf("receiving interupt\n");
        uint32_t status = readCommand(0xc0);
        if(status & 0x04)
        {
           // startLink(); /////////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!start
        }
        else if(status & 0x10)
        {
           // good threshold
        }
        else if(status & 0x80)
        {
            handleReceive();
        }
}
void package_ex( uint8_t *buf,uint16_t len){
	printf("packgae received!                                     \n");
	printf("desination mac address:                               \n");
	int i =0;
	for(i=0;i<6;i++){
	printf("%x :",*(buf+i));
	}
	printf("                              ");
	printf("\n");
	printf("source mac address:                                   \n");
	for(i=6;i<12;i++){
	printf("%x :",*(buf+i));
	}
	printf("                              ");
	printf("\n");
	printf("IP type: IPv4");
	printf("                              ");
	printf("\n");
	printf("protocol type:");
	printf("                              ");
	printf("\n");
}

void handleReceive()
{
    uint16_t old_cur;
    uint32_t got_packet = FALSE;

    while((pointer->rx_descs[pointer->rx_cur]->status & 0x1))
    {
            got_packet = TRUE;
            uint8_t *buf = (uint8_t *)pointer->rx_descs[pointer->rx_cur]->addr;
            uint16_t len = pointer->rx_descs[pointer->rx_cur]->length;

            // Here you should inject the received packet into your network stack
        #ifdef RUN_TESTS
            package_ex(buf,len);
        #endif
            package_len = len;
            package_buf = (uint8_t *)realloc(package_buf, len);
            memcpy(package_buf, buf, len);
            if((buf[278]==0x63)&&(buf[279]==0x82)&&(SEND_DHCP==0)){
            	ip[0]=buf[58];
            	ip[1]=buf[59];
            	ip[2]=buf[60];
            	ip[3]=buf[61];
            	send_dhcp_request();

            }

            pointer->rx_descs[pointer->rx_cur]->status = 0;
            old_cur = pointer->rx_cur;
            pointer->rx_cur = (pointer->rx_cur + 1) % E1000_NUM_RX_DESC;
            writeCommand(REG_RXDESCTAIL, old_cur );
    }
   // printf("here");
}

int sendPacket(const void * p_data, uint16_t p_len)
{
	pointer-> tx_descs[pointer->tx_cur]->addr = (uint32_t)p_data;
	pointer-> tx_descs[pointer->tx_cur]->length = p_len;
	pointer-> tx_descs[pointer->tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
	pointer-> tx_descs[pointer->tx_cur]->status = 0;
    uint8_t old_cur = pointer->tx_cur;
    pointer->tx_cur = (pointer->tx_cur + 1) % E1000_NUM_TX_DESC;
    //printf("%x\n",pointer->tx_cur);
    writeCommand(REG_TXDESCTAIL, pointer->tx_cur);
   // printf("%x\n",readCommand(REG_TXDESCTAIL));
    while(!(pointer->tx_descs[old_cur]->status & 0xff));
    return 0;
}
int network_init(){
    pci_initialize(); // Initialize the PCIConfigHeaderManager Object and scan PCI devices
    //printf("initialize pci complete");
    pci_device* e1000PCIConfigHeader;
    e1000PCIConfigHeader= getPCIDevice(INTEL_VEND,0x100E);
    map_virtual_4mb(e1000PCIConfigHeader->PCImem_base,e1000PCIConfigHeader->PCImem_base);
    //printf("network page setup");
    if ( e1000PCIConfigHeader != NULL )
    {
    	//NetworkDriver* e1000=&netdriver;
    	E1000(e1000PCIConfigHeader);
        if (!net_start())
        {
           printf("network start up fail");
           return 0;
        }
        return 1;
    }
    else
    {
    	printf("intel cards not found");
    	return 0;
       // Intel cards not found
    }
}
void send_arq_request(){
	arp package;
	int i = 0;
	for (i = 0;i <6;i++){
		package.dstaddress[i]=0xff;
	}
	for (i = 0;i <6;i++){
		package.srcaddress[i]=driver.mac[i];
	}
	package.arptype=switch_endian16(0x0806);
	package.htype=switch_endian16(0x1);
	package.ptype= switch_endian16(0x0800);
	package.hlen=6;
	package.plen=4;
	package.opcode=switch_endian16(0x0001);
	for (i = 0;i <6;i++){
		package.srchw[i]=driver.mac[i];
	}
	for (i = 0;i <6;i++){
		package.dsthw[i]=0xff;
	}
	package.srcpr[0]=10;
	package.srcpr[1]=192;
	package.srcpr[2]=172;
	package.srcpr[3]=205;
	package.dstpr[0]=10;
	package.dstpr[1]=192;
	package.dstpr[2]=172;
	package.dstpr[3]=205;
	sendPacket(&package,sizeof(package));
	//printf("%d",sizeof(package));
	printf("arp package send complete\n");
}

void send_dhcp_request(){
	c.a[71]=switch_endian32(0x033604c0);
	c.a[72]=switch_endian32(0x115ab432);
	c.a[73]=switch_endian32(0x04000000);
	c.a[74]=switch_endian32(0x00370a01);
	c.a[75]=switch_endian32(0x1c02030f);
	c.a[76]=switch_endian32(0x060c2829);
	c.a[77]=switch_endian32(0x2aff0000);
	c.a[73]=c.a[73]|switch_endian32((ip[0]<<16)&0x00ff0000)|switch_endian32((ip[1]<<8)&0x0000ff00)|switch_endian32(ip[2]&0x000000ff);
	c.a[74]=c.a[74]|switch_endian32(ip[3]<<24);
	c.a[73]=c.a[73]|switch_endian32(0x04000000);
	sendPacket(&c,sizeof(c));
	SEND_DHCP=1;
}






















