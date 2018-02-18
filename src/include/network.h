/*
 * network.h
 *
 *  Created on: Nov 16, 2017
 *  citation : part by http://wiki.osdev.org/Intel_Ethernet_i217;
 *      Author: jzhu62
 */
#include <pci.h>
#ifndef NETWORK_H_
#define NETWORK_H_

#define INTEL_VEND     0x8086  // Vendor ID for Intel
#define E1000_DEV      0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217     0x153A  // Device ID for Intel I217
#define E1000_82577LM  0x10EA  // Device ID for Intel 82577LM

// I have gathered those from different Hobby online operating systems instead of getting them one by one from the manual

#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818

#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818

#define REG_IP   		0x5840


#define REG_RDTR         0x2820 // RX Delay Timer Register
#define REG_RXDCTL       0x3828 // RX Descriptor Control
#define REG_RADV         0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD        0x2C00 // RX Small Packet Detect Interrupt



#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up


#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))


// Transmit Command

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable


// TCTL Register

#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun

#define TRUE							1
#define FALSE							0

#define NET_WORK_PIN					11
#define E1000_NUM_RX_DESC 				32
#define E1000_NUM_TX_DESC 				8


typedef struct arp
{
	uint8_t dstaddress[6];
	uint8_t srcaddress[6];
	uint16_t arptype;
    uint16_t htype; // Hardware type
    uint16_t ptype; // Protocol type
    uint8_t  hlen ; // Hardware address length (Ethernet = 6)
    uint8_t  plen ; // Protocol address length (IPv4 = 4)
    uint16_t opcode; // ARP Operation Code
    uint8_t  srchw[6]; // Source hardware address - hlen bytes (see above)
    uint8_t  srcpr[4]; // Source protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    uint8_t  dsthw[6]; // Destination hardware address - hlen bytes (see above)
    uint8_t  dstpr[4]; // Destination protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
}__attribute__((packed))arp;
void send_arq_request();
typedef struct ipconfig{
	uint8_t mac [6];
	uint8_t ip_address[4];
	uint8_t RCTL__EN;                             // Receiver Enable (1 << 1)
	uint8_t RCTL__SBP;                            // Store Bad Packets(1 << 2)
	uint8_t RCTL__UPE;                            // Unicast Promiscuous Enabled(1 << 3)
	uint8_t RCTL__MPE;                            // Multicast Promiscuous Enabled(1 << 4)
	uint8_t RCTL__LPE;                            // Long Packet Reception Enable(1 << 5)
	uint8_t RCTL__LBM_NONE;                       // No Loopback(0 << 6)
	uint8_t RCTL__BAM;                            // Broadcast Accept Mode (1 << 15)
	uint8_t RCTL__VFE;                            // VLAN Filter Enable (1 << 18)
	uint8_t RCTL__CFIEN;                          // Canonical Form Indicator Enable(1 << 19)
	uint8_t RCTL__DPF;                            // Discard Pause Frames(1 << 22)
	uint8_t RCTL__PMCF;                           // Pass MAC Control Frames(1 << 23)
	uint8_t RCTL__SECRC;                          // Strip Ethernet CRC(1 << 26)
	uint8_t TCTL__EN;                             // Transmit Enable(1 << 1)
	uint8_t TCTL__PSP;                            // Pad Short Packets(1 << 3)
	uint8_t TCTL__SWXOFF;                         // Software XOFF Transmission(1 << 22)
	uint8_t TCTL__RTLC;                           // Re-transmit on Late Collision(1 << 24)
}__attribute__((packed)) ipconfig;
typedef struct e1000_rx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint16_t checksum;
        volatile uint8_t status;
        volatile uint8_t errors;
        volatile uint16_t special;
} __attribute__((packed)) e1000_rx_desc;

typedef struct e1000_tx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint8_t cso;
        volatile uint8_t cmd;
        volatile uint8_t status;
        volatile uint8_t css;
        volatile uint16_t special;
} __attribute__((packed)) e1000_tx_desc;

typedef struct c{
	uint32_t a[85];
	uint16_t b;
}__attribute__((packed))dhcp;

typedef struct  network_driver
{

        uint8_t bar_type;     // Type of BOR0
        uint16_t io_base;     // IO Base Address
        uint32_t  mem_base;   // MMIO Base Address
        uint16_t eerprom_exists;  // A flag indicating if eeprom exists
        uint8_t mac [6];      // A buffer for storing the mack address
        struct e1000_rx_desc *rx_descs[E1000_NUM_RX_DESC]; // Receive Descriptor Buffers
        struct e1000_tx_desc *tx_descs[E1000_NUM_TX_DESC]; // Transmit Descriptor Buffers
        uint16_t rx_cur;      // Current Receive Descriptor Buffer
        uint16_t tx_cur;      // Current Transmit Descriptor Buffer                            // Default Destructor
}__attribute__((packed))NetworkDriver;

extern NetworkDriver* pointer;

// Send Commands and read results From NICs either using MMIO or IO Ports
// Send Commands and read results From NICs either using MMIO or IO Ports
int network_init();
void writeCommand( uint16_t p_address, uint32_t p_value);
uint32_t readCommand(uint16_t p_address);
uint32_t _detectEEProm(); // Return true if EEProm exist, else it returns false and set the eerprom_existsdata member
uint32_t eepromRead( uint8_t addr); // Read 4 bytes from a specific EEProm Address
uint32_t readMACAddress();       // Read MAC Address
void startLink ();           // Start up the network
void rxinit();               // Initialize receive descriptors an buffers
void txinit();               // Initialize transmit descriptors an buffers
void enableInterrupt();      // Enable Interrupts
void handleReceive();        // Handle a packet reception.
void E1000(pci_device * pci_device); // Constructor. takes as a parameter a pointer to an object that encapsulate all he PCI configuration data of the device
uint32_t net_start ();                             // perform initialization tasks and starts the driver
void fire ();  // This method should be called by the interrupt handler
uint8_t * getMacAddress ();                         // Returns the MAC address
int sendPacket(const void * p_data, uint16_t p_len);  // Send a packet
void free_E1000();// Default Destructor
void send_dhcp_request();
int32_t get_package(void * buf, uint32_t nbytes);
int32_t get_ipconfig(void*buf);
int32_t get_ip();

#endif /* NETWORK_H_ */
