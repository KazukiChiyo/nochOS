#include <tests.h>
#include <x86_desc.h>
#include <lib.h>
#include <mod_fs.h>
#include <types.h>
#include <syscall.h>
#include <proc.h>
#include <pci_ide.h>
#include <terminal.h>
#include <sound.h>
#include <ext2.h>
#include <mem.h>
#include <list.h>
#include <aes.h>
#include <rand.h>
#include <signal.h>
#include <network.h>

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

///
/// This test function divides a number by zero.
///
/// - author: Zhengcheng Huang
/// - return: The returned value is 10/0, which should raise an exception on
///           execution
/// - coverage: IDT entry initialization, exception handling.
/// - files: kernel.c, x86_desc.S
///
// int idt_divide_test() {
//    TEST_HEADER;
//
//     return 10 / 0;
// }

///
/// This test function attempts to access a page that does not exist.
///
/// - author: Zhengcheng Huang
/// - return: The return value is to be retrieved from a page that is not
///           present, so a page fault exception should be raised.
/// - coverage: Paging initialization.
/// - files: include/paging.h, paging.c, asm/paging.S
///
int page_fault_test() {
    int magic;

    asm volatile ("movl 0xFFFFFFFF,%eax");

    return magic;
}

///
/// This test function attempts to dereference a pointer cast from an
/// integer representing an address. For valid virtual addresses, this
/// function should return the value in that location. For invalid
/// virtual address, this function should raise a page fault exception.
///
/// - coverage: Paging initialization.
/// - files: include/paging.h, paging.c, asm/paging.S
///
int page_fault_test2() {
    return (int)(*((int *)0));
}

///
/// This test function prints information read from the boot block.
///
/// - coverage: File system interfaces.
/// - files: include/fs.h, fs.c
///
int test_bb() {
/*
    printf("# inodes: %d\n", boot_block->inodes);
    printf("# dentries: %d\n", boot_block->dir_entries);
    printf("# data blocks: %d\n", boot_block->data_blocks);
    return 0;
*/
    return 0;
}

///
/// *** This function is deprecated because we can already run the ls program.
///
/// This test function prints all dentries in the file system except ".".
///
/// - coverage: File system interfaces.
/// - files: include/fs.h, fs.c
///
int test_ls() {
    uint32_t fd, cnt;
    uint8_t buf[34];

    fd = sys_open((int8_t *)".");

    while (0 != (cnt = sys_read(fd, buf, 32))) {
        if (-1 == cnt) {
            printf("Directory entry read failed\n");
            return 3;
        }
        buf[cnt] = '\n';
        printf("File name: %s\n", buf);
    }

    return 0;
}

///
/// *** This test is deprecated because we can already run the fish and cat
///     program to test the functionality.
///
/// This test function prints information of file "frame0.txt", including
/// all of its content.
///
/// - coverage: File system interfaces.
/// - files: include/fs.h, fs.c
///
int test_iblock_addr() {
/*
    uint32_t fd, bytes;
    uint8_t data[202];

    uint32_t inode_num, inode_addr, data_addr;

    fd = sys_open((uint8_t *)"frame0.txt");

    inode_num = cur_proc_pcb->fd_array[fd].inode;
    inode_addr = (uint32_t)boot_block + (inode_num + 1) * MOD_FBLOCK_SIZE;
    data_addr = (uint32_t)boot_block + 65 * MOD_FBLOCK_SIZE;
    printf("FS addr    --- %x\n", (uint32_t)boot_block);
    printf("Data addr  --- %x\n", (uint32_t)data_addr);
    printf("Inode #    --- %d\n", inode_num);
    printf("Inode addr --- %x\n", inode_addr);

    bytes = sys_read(fd, data, 200);
    data[bytes] = '\n';
    data[bytes + 1] = '\0';

    printf("%s", data);
*/

    return 0;
}

///
/// *** This test is deprecated because we can already test its functionality
///     by running cat and other programs that reads files.
///
/// This test function prints the entire large file.
///
/// - coverage: File system interfaces.
/// - files: include/fs.h, fs.c
///
int test_fread() {
    /*
    clear_screen();
    uint32_t fd, bytes;
    uint8_t data[10002];

    fd = sys_open((uint8_t *)"verylargetextwithverylongname.txt");

    bytes = sys_read(fd, data, 10000);
    data[bytes] = '\0';

    //printf("Data: %s\n", data);
    sys_write(1, data, bytes);
*/
    return 0;
}

///
/// This test function prints the entry point of the executable
/// file.
///
/// - coverage: File system interfaces.
/// - files: include/fs.h, fs.c
///
int test_read_exec() {
    uint8_t data [5];

    mod_dentry_t dentry;
    read_dentry_by_name((int8_t *)"ls", &dentry);
    read_data(dentry.inode, 24, data, 4);
    data[4] = '\n';

    printf("Data: %x\n", data[0]);
    printf("Data: %x\n", data[1]);
    printf("Data: %x\n", data[2]);
    printf("Data: %x\n", data[3]);

    return 0;
}

///
/// *** This test is deprecated because we can already test the functionaliry
///     by running pingpong or fish.
///
/// This test function prints in rhythm according to the RTC frequency.
/// RTC's frequency will be changed after printing the first 8 lines.
///
/// - coverage: RTC read/write operations.
/// - files: include/rtc.h, rtc.c
///
int test_rtc() {
    uint32_t fd, freq;
    uint32_t max, count;

    max = 8;
    count = 0;

    freq = 4;
    fd = sys_open((int8_t *)"rtc");

    sys_write(fd, &freq, 4);

    while (count < max) {
        sys_read(fd, 0, 4);
        sys_write(1, "RTC says hi.\n", 13);
        count++;
    }

    freq = 8;
    sys_write(fd, &freq, 4);

    while (1) {
        sys_read(fd, 0, 4);
        sys_write(1, "RTC's boy says hi.\n", 19);
    }

    return 0;
}

///
/// *** This test is deprecated because we can test the functionality
///     in various user programs.
///
int test_write() {
    sys_write(1, (uint8_t *)"Bad coder codes bad code.\n", 26);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    sys_write(1, (uint8_t *)"Good coder codes bad code.\n", 27);
    return 0;
}

void test_ide_init(void) {
    int i;
    uint8_t good_buf [1024];
    uint8_t bad_buf [4096];

    for (i = 0; i < 4096; i++) {
        bad_buf[i] = 0xA5;
    }

    //ide_ata_access(1, 0, 0x0, 1, KERNEL_DS, bad_buf);
    ide_device->d_op->read(ide_device, good_buf, 0x41, 513);
    for (i = 0; i < 40; i++) {
        printf("%x ", good_buf[i]);
    }
    printf("\n");
}

void test_ext2_parse(void) {
    uint8_t good_buf [1024];
    ext2_super_t * sb;
    ide_init();
    ide_device->d_op->read(ide_device, good_buf, 0x41, 1024);
    sb = (ext2_super_t *)good_buf;

    printf("Revision level: %d\n", sb->s_rev_level);
    printf("Block size: %d\n", 1024 << sb->s_log_block_size);
    printf("Inode size: %d\n", sb->s_inode_size);
    printf("Inode count: %d\n", sb->s_inodes);
    printf("Block count: %d\n", sb->s_blocks);
    printf("Free inodes: %d\n", sb->s_free_inodes);
    printf("Free blocks: %d\n", sb->s_free_blocks);
    printf("Inodes per block: %d\n", sb->s_inodes_per_group);
}

void test_ext2_read_root(void) {
    int i = 0;
    int32_t res;
    file_t file;
    int8_t buf [33];

    file.f_op = ext2_dir_fop;
    file.f_op->open(&file, ".");

    while (0 != (res = file.f_op->read(&file, buf, 32))) {
        printf("------- File %d -------\n", i);
        buf[res] = '\0';
        printf("  Filename: %s\n", buf);
        printf("  Filetype: %x\n", file.f_dentry.d_inode.i_mode);
        i++;
    }
}

void test_ext2_read_file(void) {
    uint8_t file_buf [1024];
    int32_t bytes;
    file_t file;

    file.f_op = ext2_file_fop;
    file.f_op->open(&file, "frame0.txt");

    bytes = file.f_op->read(&file, file_buf, 1024);
    file_buf[bytes] = '\0';
    printf("%s\n", file_buf);
}

void test_ext2_write_file(void) {
    char file_buf [1024];
    char * file_text;
    int bytes;
    file_t file;
    file.f_op = ext2_file_fop;
    file.f_op->open(&file, "frame0.txt");

    file_text = "Many years later, when...\n";

    file.f_op->write(&file, file_text, 26);
    file.f_pos = 0;
    bytes = file.f_op->read(&file, file_buf, 1024);
    file_buf[bytes] = '\0';
    printf("%s\n", file_buf);
}

void test_ext2_touch_and_write(void) {
    char file_buf [1024];
    char * file_text;
    int32_t bytes;
    file_t file;
    inode_t * iroot;
    dentry_t dentry;

    strcpy(dentry.filename, "solitude.txt");

    iroot = &superblock.s_root.d_inode;

    file_text = "Many years later, when...\n";

    iroot->i_op->create(iroot, &dentry, 0x81FF);

    file.f_op = ext2_file_fop;
    file.f_op->open(&file, "solitude.txt");
    file.f_op->write(&file, file_text, 26);
    file.f_pos = 0;
    bytes = file.f_op->read(&file, file_buf, 1024);
    file_buf[bytes] = '\0';
    printf("%s\n", file_buf);
}

void test_ext2_rm(void) {
    inode_t * iroot;
    int8_t * fname;

    fname = "nooo.txt";
    iroot = &superblock.s_root.d_inode;

    iroot->i_op->remove(iroot, fname);
}

typedef struct irq_action {
    void (* handler)(void);
    uint32_t flags;
    uint32_t mask;
    uint8_t name[32];
    uint32_t dev_id;
    list_head node;
    uint32_t irq;
    uint32_t* dir;
} __attribute__((packed)) irq_action;

void kindratenko_handler(void) {
    printf("Vladimir!\n");
}

void patel_handler(void) {
    printf(".......\n");
}

void lumetta_handler(void) {
    printf("What are you babbling about?\n");
}

void torvalds_handler(void) {
    printf("***Censored***\n");
}

void test_bst(void) {
    irq_action * kindratenko_action = malloc(sizeof(irq_action));
    irq_action * patel_action = malloc(sizeof(irq_action));
    irq_action * lumetta_action = malloc(sizeof(irq_action));
    irq_action * torvalds_action = malloc(sizeof(irq_action));

    kindratenko_action->handler = kindratenko_handler;
    patel_action->handler = patel_handler;
    lumetta_action->handler = lumetta_handler;
    torvalds_action->handler = torvalds_handler;

    list_head * head = malloc(sizeof(list_head));

    list_insert_after(&kindratenko_action->node, head);
    list_insert_after(&patel_action->node, &kindratenko_action->node);
    list_insert_after(&lumetta_action->node, &patel_action->node);
    list_insert_after(&torvalds_action->node, &lumetta_action->node);
    torvalds_action->node.next = head;

    irq_action * itr;
    list_entry_itr(itr, head, node) {
        itr->handler();
    }

    free(kindratenko_action);
    free(patel_action);
    free(lumetta_action);
    free(torvalds_action);
    free(head);
}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


int test_sound(){
    int a[16] = {0 ,1 ,2 ,3 ,4 ,5 ,6 , 7 ,6, 5, 4, 3, 2, 1};
    int i =0 ;
    for(i=0;i<16;i++)
    {
        int number= (a[i]+2)*100;
        beep(number,4);
    }
    return 0;
}


void test_rand(void) {
    int i, j;
    uint8_t key[16];
    for (i = 0; i < 10; i++) {
        aes_get_random_key(key);
        printf("Key round %d:\n", i);
        for (j = 0; j < 16; j++)
            printf("%x ", key[j]);
        printf("\n");
    }
}


void test_aes_block_encryption(void) {
    int i;
    uint8_t key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    key_expansion(key);
    uint8_t plaintext[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t en_text[16];
    aes_block_encryption(plaintext, en_text);
    printf("Encrypted key is:\n");
    for (i = 0; i < 16; i++)
        printf("%x ", en_text[i]);
    printf("\n");
}


void test_aes_block_decryption(void) {
    int i;
    uint8_t key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    key_expansion(key);
    uint8_t encrypted[16] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
    uint8_t de_text[16];
    aes_block_decryption(encrypted, de_text);
    printf("Decrypted key is:\n");
    for (i = 0; i < 16; i++)
        printf("%x ", de_text[i]);
    printf("\n");
}


/* 10 mb data, AES_FAST_CALC: 12.10s; else: 13.12s */
void test_aes_runtime(void) {
    int i;
    uint8_t key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    uint8_t plaintext[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    key_expansion(key);
    uint8_t en_text[16];
    printf("start counting\n");
    for (i = 0; i < 625000; i++) // 10 mb data
        aes_block_encryption(plaintext, en_text);
    printf("finish counting\n");

}


void test_sigqueue(void) {
    int i;
    sigaction* new_item = NULL;
    list_head head;
    init_list_head(&(head));
    for (; i < 10; i++) {
        new_item = malloc(sizeof(sigaction));
        new_item->sig = i;
        printf("now enqueue %d\n", i);
        enqueue_signal(new_item, &(head));
    }
}


/* Test suite entry point */
void launch_tests(){
	// launch your tests here

    //test_bst();

	send_arq_request();
	//while(1);
	send_dhcp_request();

    while (1);

}
