#include <aes.h>
#include <vfs.h>
#include <rtc.h>
#include <types.h>
#include <proc.h>
#include <paging.h>
#include <lib.h>
#include <x86_desc.h>
#include <terminal.h>
#include <ext2.h>
#include <regs.h>
#include <signal.h>
#include <syscall.h>
#include <system.h>
#include <time.h>
#include <mem.h>
#include <aes.h>
#include <network.h>
#include <color.h>

pcb_t * cur_proc_pcb; // declared in proc.h
uint8_t proc_status[NUM_PROC]; // declared in proc.h
sess_t sess_desc[NUM_SESS]; // declared in terminal.h
uint32_t cur_sess_id; // declared in terminal.h

/* executable file identifier, must match a given file's 0-3 bytes */
static int8_t is_exe[CMD_WORD_SIZE] = {0x7F, 0x45, 0x4C, 0x46};

static struct regs * cur_regs;

/**
 * do_sys - handles an interrupt for specific system call
 * @param regs - register information stored on top of the stack
 */
__attribute__((fastcall))
void do_sys(struct regs *regs) {
    CHK_USR_ESP;
    int32_t retval;
    uint32_t sysnum = regs->orig_eax;
    cur_regs = regs;
    switch (sysnum) {
        case SYS_HALT:
            retval = sys_halt((uint8_t)regs->ebx);

        case SYS_EXECUTE:
            retval = sys_execute((const int8_t* )regs->ebx);

        case SYS_READ:
            retval = sys_read((uint32_t)regs->ebx, (void* )regs->ecx, (int32_t)regs->edx);
            break;

        case SYS_WRITE:
            retval = sys_write((uint32_t)regs->ebx, (const void* )regs->ecx, (int32_t)regs->edx);
            break;

        case SYS_OPEN:
            retval = sys_open((const int8_t* )regs->ebx);
            break;

        case SYS_CLOSE:
            retval = sys_close((uint32_t)regs->ebx);
            break;

        case SYS_GETARGS:
            retval = sys_getargs((int8_t* )regs->ebx, (int32_t)regs->ecx);
            break;

        case SYS_VIDMAP:
            retval = sys_vidmap((uint8_t** )regs->ebx);
            break;

        case SYS_SET_HANDLER:
            retval = sys_set_handler((int32_t)regs->ebx, (void* )regs->ecx);
            break;

        case SYS_SIGRETURN:
            retval = sys_sigreturn();
            break;

        case SYS_KILL:
            retval = sys_kill((uint32_t)regs->ebx, (int32_t)regs->ecx);
            break;

        case SYS_QUERY:
            retval = sys_query((uint32_t)regs->ebx, (proc_info*)regs->ecx);
            break;

        case SYS_INFO:
            retval = sys_info((system_info*)regs->ebx);
            break;

        case SYS_CREATE:
            retval = sys_create((const int8_t *)regs->ebx);
            break;

        case SYS_RM:
            retval = sys_rm((const int8_t *)regs->ebx);
            break;

        case SYS_MKDIR:
            retval = sys_mkdir((const int8_t *)regs->ebx);
            break;

        case SYS_CD:
            retval = sys_cd((const int8_t *)regs->ebx);
            break;

        case SYS_SEEK:
            retval = sys_seek((uint32_t)regs->ebx, (int32_t)regs->ecx, (int32_t)regs->edx);
            break;

        case SYS_ENCRYPT:
            retval = sys_encrypt((const int8_t *)regs->ebx);
            break;

        case SYS_DECRYPT:
            retval = sys_decrypt((const int8_t *)regs->ebx);
            break;

        case SYS_FILEMODE:
            retval = sys_filemode((uint32_t)regs->ebx);
            break;

        case SYS_PWD:
            retval = sys_pwd((int8_t *)regs->ebx);
            break;

        case SYS_NET_PACKAGE:
            retval = sys_net_package((void *)regs->ebx, (uint32_t)regs->ecx);
            break;

        case SYS_SHUTDOWN:
            retval = sys_shutdown();
            break;

        case SYS_SETUSR:
            retval = sys_setusr((const uint8_t* )regs->ebx);
            break;

        case SYS_GETUSR:
            retval = sys_getusr((uint8_t* )regs->ebx);
            break;

        case SYS_GETPID:
            retval = sys_getpid();
            break;

        case SYS_TEXTCOLOR:
            retval = sys_textcolor((uint8_t)regs->ebx);
            break;

        case SYS_MAP_MODEX:
            retval = sys_map_modex((uint8_t** )regs->ebx);
            break;

        case SYS_IPCONFIG:
        	retval = sys_ipconfig((void *)regs->ebx);
        	break;
        case SYS_GETIP:
        	retval = sys_getip();
        	break;
        default: return;
    }
    regs->eax = retval;
}


/* sys_halt
   description: write to terminal screen
   input: status - status of execute function
   output: none
   return value: status of execute funciton, pass by %eax
   side effect: none
   author: Kexuan Zou
*/
int32_t sys_halt(int32_t status) {
    kill_pid(cur_proc_pcb->pid, status);
    return 0;
}


/* sys_execute
   description: write to terminal screen
   input: command - [filename] [arg]
   output: none
   return value: nullfied by iret
   side effect: none
   author: Kexuan Zou
*/
int32_t sys_execute(const int8_t * command) {
    /* if cpu reaches its maximum control, abort */
    if (cur_proc_pcb && get_next_pid(cur_proc_pcb->pid, INACTIVE) == NUM_PROC)
        return -1;
    uint32_t entry_point;
    file_t program;
    int8_t parsed_cmd[ARG_WORD_SIZE];
    int8_t * bin_fname;
    uint8_t cmd_buf[CMD_WORD_SIZE];
    int32_t cmd_status;
    int8_t fout [FNAME_LEN];
    if (cur_proc_pcb)
        proc_status[cur_proc_pcb->pid] = IDLE; // switch parent process to idle

    /* parse the command to get the first field */
    //clear_args(cur_proc_pcb);
    cmd_status = parse_cmd(command, parsed_cmd);
    if (cmd_status == -1) {
        proc_status[cur_proc_pcb->pid] = DAEMON;
        return -1; // if command is invalid return
    }

    /* find file based on the parsed command */
    bin_fname = (int8_t *)malloc(cmd_status + 5);
    strcpy(bin_fname, "/bin/");
    strcpy(bin_fname + 5, parsed_cmd);
    program.f_op = ext2_file_fop;
    if (-1 == program.f_op->open(&program, parsed_cmd)) {
        if (-1 == program.f_op->open(&program, bin_fname) || parsed_cmd[0] == '.') {
            proc_status[cur_proc_pcb->pid] = DAEMON;
            return -1;
        }
    }

    /* read file and check if it is a valid executable */
    program.f_pos = 0;
    program.f_op->read(&program, cmd_buf, CMD_WORD_SIZE);
    if (strncmp((int8_t *)cmd_buf, is_exe, CMD_WORD_SIZE)) {
        proc_status[cur_proc_pcb->pid] = DAEMON;
        return -1;
    }

    /* read entry point */
    program.f_pos = ENTRY_POINT_POS;
    program.f_op->read(&program, cmd_buf, CMD_WORD_SIZE);
    entry_point = (uint32_t)cmd_buf[0] | ((uint32_t)cmd_buf[1] << 8) | ((uint32_t)cmd_buf[2] << 16) | ((uint32_t)cmd_buf[3] << 24);

    /* set up child pcb */
    pcb_t* child_pcb;
    child_pcb = child_pcb_init(cur_proc_pcb); // initialize child pcb

    /* update pcb to current process */
    cur_proc_pcb = child_pcb;

    /* link sigaction linkage pcb */
    link_sa_pcb(cur_proc_pcb, cur_sess_id);
    proc_signal_init(cur_proc_pcb);

    /* clear argument field and set up argument relative to current process */
    fout[0] = '\0';
    strncpy(cur_proc_pcb->command, parsed_cmd, ARG_WORD_SIZE);
    parse_arg(command, cur_proc_pcb, fout, cmd_status + 1);

    /* set up file descriptor, enables stdin and stdout */
    fd_array_init();

    file_io_init("", fout);

    /* set up parent_esp and parent_ebp. parent_esp is used by 'iret' in sys_halt() and must point to return address set up when 'int 0x80' was made. parent_ebp is the ebp of current stack frame. */
    cur_proc_pcb->parent_esp = (uint32_t)cur_regs;

    /* initilize a 4mb page for user program; maps program physical address to 128MB, copy data to the virtual memory space */
    uint32_t prog_phys_addr = get_phys_addr_by_pid(cur_proc_pcb->pid);
    map_virtual_4mb(prog_phys_addr, USER_VIRT_TOP);
    program.f_pos = 0;
    program.f_op->read(&program, (void *)PROG_VIRT_START, program.f_dentry.d_inode.i_size);
    SET_BRK(PROG_VIRT_START + program.f_dentry.d_inode.i_size);
    STACK_BARRIER;
    program.f_op->close(&program);

    /* if switching to current session from daemon session, then current process running must switch back to active and write to video memory */
    if (cur_proc_pcb->active_sess == cur_sess_id)
        map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);

    /* if switching away from current session, then current process running must run as daemon and write to its assigned cached video memory */
    else
        map_virtual_4kb_prog(sess_desc[cur_sess_id].cached_vidmem, VMEM_VIRT_START);

    /* modify tss. ss0 is kernel stack segment, esp0 points to the starting address of current process's kernel stack */
    tss.esp0 = get_esp0_by_pid(cur_proc_pcb->pid);

    /* set video memory */
    set_vidmem_param(VMEM_VIRT_START);

    /* fake iret to user program's user stack. SS is user stack segment, ESP points to the starting address of current process's user stack, EFLAGS are never used, CS is user code segment, return address (in this case eip of user program) is the entry point specified by each file */
    asm volatile (
        "pushl %0\n" // push SS
        "pushl %1\n" // push ESP
        "pushfl\n" // push EFLAGS
        "orl $0x200,(%%esp)\n"
        "pushl %2\n" // push CS
        "pushl %3\n" // push return address
        "iret\n"
        : // output operands
        : "g"(USER_DS), "g"(USER_VIRT_BOT - 4), "g"(USER_CS), "g"(entry_point) // input operands
    );
    return 0;
}


/* sys_getargs
   description: pass argument relative to this process to a user buffer
   input: buf - user buffer
          nbytes - bytes to copy
   output: none
   return value: -1 if cannot read argument or it is invalid, 0 if success
   side effect: none
   author: Kexuan Zou
*/
int32_t sys_getargs(int8_t* buf, int32_t nbytes) {
    if (buf == NULL) return -1; // if buffer is invalid, return -1
    if (cur_proc_pcb->arg[0] == '\0') return -1; // if argument buffer is empty, return -1
    strncpy(buf, cur_proc_pcb->arg, nbytes);
    return 0;
}


/* sys_vidmap
   description: map physical video memory address to a designated one
   input: screen_start - starting screen address
          nbytes - bytes to copy
   output: none
   return value: -1 if cannot read argument or it is invalid, 0 if success
   side effect: none
   author: Kexuan Zou
*/
int32_t sys_vidmap(uint8_t** screen_start) {
    if (screen_start == NULL) return -1; // if pointer is invalid, return
    if ((uint32_t)screen_start < USER_VIRT_TOP || (uint32_t)screen_start >= USER_VIRT_BOT)
        return -1; // if screen starting address is not within the user program's 4MB space return -1
    map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);
    *screen_start = (uint8_t* )VMEM_VIRT_START;
    return 0;
}


/**
 * sys_set_handler - set user-defined handler for signal
 * @param signum - signal number to set
 * @param handler_address - pointer to handler address
 * @return - 0 if success, -1 if fail
 * @author - Kexuan Zou
 */
int32_t sys_set_handler(int32_t signum, void* handler_address) {
    if (signum < DIV_ZERO || signum > USER1) return -1; // if signum is invalid
    GET_SA_HANDLER(signum, cur_proc_pcb) = (uint32_t) handler_address;
    GET_SA_MASK(signum, cur_proc_pcb) = SIG_MASK_CLEAR;
    return 0;
}


/**
 * sys_sigreturn - return a signal
 * @return - return status from sys_halt
 * @author - Kexuan Zou
 */
int32_t sys_sigreturn(void) {
    /* copy hardware context from user stack back to kernel stack */
    memcpy((void* )cur_regs, (const void* )cur_regs->esp, sizeof(struct regs));

    /* if there are pending signals, replay */
    if (!list_is_empty(&(cur_proc_pcb->sigpending)))
        do_signal(cur_regs);
    return cur_regs->eax;
}


/// Reads a file with given file descriptor.
///
/// - author: Zhengcheng Huang
///
int32_t sys_read(uint32_t fd, void * buf, uint32_t nbytes) {
    file_t * file;

    // Check if file descriptor in within range.
    if (fd >= MAX_OPEN_FILES)
        return -1;

    if (fd_avail(fd))
        return -1;

    file = cur_proc_pcb->fd_array + fd;

    return file->f_op->read(file, buf, nbytes);
}


///
/// Writes a file with given file descriptor.
///
/// - author: Zhengcheng Huang
///
int32_t sys_write(uint32_t fd, const void * buf, uint32_t nbytes) {
    file_t * file;

    // Check if file dexcriptor is within range.
    if (fd >= MAX_OPEN_FILES)
        return -1;

    if (fd_avail(fd))
        return -1;

    file = cur_proc_pcb->fd_array + fd;

    return file->f_op->write(file, buf, nbytes);
}


///
/// Opens a file with given filename.
///
/// - author: Zhengcheng Huang
///
int32_t sys_open(const int8_t * filename) {
    uint32_t fd;
    int available;
    file_t * file;

    // Find next available fd.
    available = 0;
    for (fd = 2; fd < MAX_OPEN_FILES; fd++) {
        if (fd_avail(fd)) {
            available = 1;
            break;
        }
    }

    // If fd array is full, return failure.
    if (!available)
        return -1;

    // Get file operation jumptable according to file type.
    file = cur_proc_pcb->fd_array + fd;
    if (0 == strncmp(filename, "rtc", FNAME_LEN))
        file->f_op = rtc_fop;
    else {
        file->f_op = ext2_file_fop;
    }
    // Open the file. This operation may fail.
    // On failure, flags for the fd will not be set.
    if (file->f_op->open(file, filename) != 0)
        return -1;

    cur_proc_pcb->fd_bitmap |= 1 << fd;
    return fd;
}


///
/// Closes a file with given file descriptor.
///
/// - author: Zhengcheng Huang
///
int32_t sys_close(uint32_t fd) {
    int32_t retval;
    file_t * file;

    // Check if file descriptor is within range.
    if (fd >= MAX_OPEN_FILES)
        return -1;

    if (fd_avail(fd))
        return -1;

    file = cur_proc_pcb->fd_array + fd;
    retval = file->f_op->close(file);
    if (retval == -1)
        return -1;
    else {
        cur_proc_pcb->fd_bitmap &= ~(1 << fd);
        return 0;
    }
}


/**
 * sys_kill - kill a process given its pid
 * @param pid - pid to kill
 * @param ignore - signum to kill; we have only SIGKILL to kill a process so this field is trivial
 * @return - no real effect
 * @author - Kexuan Zou
 */
int32_t sys_kill(uint32_t pid, int32_t ignore) {
    request_signal(SIGKILL, get_pcb_by_pid(pid));
    return 0; // trivial return value, control sequence never reach here
}


/**
 * sys_query - query a process' status
 * @param pid - the process' pid
 * @param obj - process' status object
 * @return - 0 if process exists, -1 if process does not exist
 */
int32_t sys_query(uint32_t pid, proc_info* obj) {
    if (proc_status[pid] == INACTIVE) return -1;
    pcb_t* cur_pcb = get_pcb_by_pid(pid);
    strncpy(obj->cmd, cur_pcb->command, ARG_WORD_SIZE);
    if (proc_status[pid] == ACTIVE || proc_status[pid] == DAEMON)
        obj->status = ACTIVE;
    else
        obj->status = IDLE;
    obj->uptime = cur_pcb->uptime;
    return 0;
}


/**
 * sys_info - return system's info
 * @param obj - system's status object
 * @return - 0, auto success
 */
int32_t sys_info(system_info* obj) {
    obj->num_active = query_proc_status(ACTIVE) + query_proc_status(DAEMON);
    obj->num_idle = query_proc_status(IDLE);
    obj->uptime = system_time.count_ms;
    return 0;
}


int32_t sys_create(const int8_t * fname) {
    dentry_t * base, * dent;
    int32_t retval;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));
    if (-1 == (retval = parse_path(fname, base, dent))) {
        if (retval != -1)
            free_dentry(dent);
        return -1;
    }

    retval = base->d_inode.i_op->create(&base->d_inode, dent, 0x81FF);
    free_dentry(dent);
    return retval;
}

int32_t sys_rm(const int8_t * fname) {
    dentry_t * base, * dent;
    int32_t retval;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));
    if (0 != (retval = parse_path(fname, base, dent))) {
        if (retval != -1)
            free_dentry(dent);
        return -1;
    }

    if (dent->d_inode.i_ino == cur_dentry.d_inode.i_ino)
        return -1;

    retval = base->d_inode.i_op->remove(&base->d_inode, dent->filename);
    free_dentry(dent);
    return retval;
}

int32_t sys_mkdir(const int8_t * dir_name) {
    dentry_t * base, * dent;
    int32_t retval;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));

    if (1 != (retval = parse_path(dir_name, base, dent))) {
        if (retval != -1)
            free_dentry(dent);
        return -1;
    }

    retval = base->d_inode.i_op->mkdir(&base->d_inode, dent->filename);
    free_dentry(dent);
    return retval;
}

int32_t sys_cd(const int8_t * dir_name) {
    dentry_t * base, * dent;
    int32_t res;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));

    if (0 != (res = parse_path(dir_name, base, dent))) {
        if (res == 1)
            free_dentry(dent);
        return -1;
    }

    if (dent->filename[0] == '/')
        dent = &superblock.s_root;

    if (dent->d_parent->filename[0] == '/' && dent->d_parent != &superblock.s_root) {
        free(dent->d_parent);
        dent->d_parent = &superblock.s_root;
    }

    if (0x4 != ((dent->d_inode.i_mode >> 12) & 0xF)) {
        free_dentry(dent);
        return -1;
    }
/*
    if (-1 == base->d_inode.i_op->lookup(&base->d_inode, dent, dent->filename)) {
        free_dentry(dent);
        return -1;
    }
*/
    free_dentry(cur_dentry.d_parent);
    cur_dentry = *dent;
    if (dent != &superblock.s_root)
        free(dent);
    return 0;
}

int32_t sys_seek(uint32_t fd, int32_t offset, int32_t whence) {
    file_t * file;

    // check if file descriptor in within range.
    if (fd >= MAX_OPEN_FILES)
        return -1;

    if (fd_avail(fd))
        return -1;

    file = cur_proc_pcb->fd_array + fd;

    return file->f_op->seek(file, offset, whence);
}

int32_t sys_encrypt(const int8_t * fname) {
    file_t file;
    uint32_t fsize, bytes, i, length;
    int8_t * content;
    uint8_t key [16], in [16], out [16];
    file.f_op = ext2_file_fop;

    file.f_op->open(&file, fname);

    aes_get_random_key(key);
    file.f_op->setkey(&file, key);
    key_expansion(key);

    fsize = file.f_dentry.d_inode.i_size;
    bytes = fsize / 16;
    content = malloc(fsize);

    for (i = 0; i < bytes + 1; ++i) {
        length = file.f_op->read(&file, in, 16);
        for (; length < 16; ++length)
            in[length] = '\0';

        aes_block_encryption(in, out);
        memcpy(content + i * 16, out, 16);
    }

    file.f_pos = 0;
    for (i = 0; i < bytes + 1; ++i) {
        file.f_op->write(&file, content + i * 16, 16);
    }

    return 0;
}

int32_t sys_decrypt(const int8_t * fname) {
    file_t file;
    uint32_t fsize, bytes, i, length;
    int8_t * content;
    uint8_t key [16], in [16], out [16];
    file.f_op = ext2_file_fop;

    file.f_op->open(&file, fname);
    file.f_op->getkey(&file, key);
    key_expansion(key);

    fsize = file.f_dentry.d_inode.i_size;
    bytes = fsize / 16;
    content = malloc(fsize);

    for (i = 0; i < bytes; ++i) {
        file.f_op->read(&file, in, 16);
        aes_block_decryption(in, out);
        memcpy(content + i * 16, out, 16);
    }

    file.f_pos = 0;
    for (i = 0; i < bytes - 1; ++i) {
        file.f_op->write(&file, content + i * 16, 16);
    }

    for (length = 16; length > 0; --length) {
        if (content[i * 16 + length - 1] != '\0')
            break;
    }
    file.f_op->write(&file, content + i * 16, length);
    return 0;
}

int32_t sys_filemode(uint32_t fd) {
    file_t * file;

    // check if file descriptor in within range.
    if (fd >= MAX_OPEN_FILES)
        return -1;

    if (fd_avail(fd))
        return -1;

    file = cur_proc_pcb->fd_array + fd;
    return file->f_dentry.d_inode.i_mode;
}

int32_t sys_pwd(int8_t * path) {
    int8_t * pwd_path, * swp_path;
    uint32_t path_len, f_len;
    dentry_t * dp = &cur_dentry;

    if (dp->filename[0] == '/') {
        path[0] = '/';
        return 1;
    }

    pwd_path = malloc(1);
    pwd_path[0] = '\0';
    path_len = 0;

    do {
        f_len = strlen(dp->filename);
        if (FNAME_LEN < f_len)
            f_len = FNAME_LEN;
        swp_path = pwd_path;
        pwd_path = malloc(path_len + f_len + 1 + 1);
        pwd_path[0] = '/';
        strncpy(pwd_path + 1, dp->filename, f_len);
        strncpy(pwd_path + 1 + f_len, swp_path, path_len);
        pwd_path[path_len + f_len + 1] = '\0';
        path_len = path_len + f_len + 1;
        free(swp_path);
        dp = dp->d_parent;
    } while (dp->filename[0] != '/');

    strncpy(path, pwd_path, path_len);
    path[path_len] = '\0';
    free(pwd_path);
    return path_len + 1;
}

int32_t sys_net_package(void * buf, uint32_t nbytes) {
    return get_package(buf, nbytes);
}
int32_t sys_ipconfig(void *buf){
	return get_ipconfig(buf);
}


/**
 * sys_shutdown - shutdown the system
 * @return - 0, auto success
 */
int32_t sys_shutdown(void) {
    PWR_OFF;
    return 0; // control sequence never reaches here
}


/**
 * sys_setusr - set user name
 * @param usr - user name
 * @return - 0, auto success
 */
int32_t sys_setusr(const uint8_t * usr) {
    strncpy((int8_t* )usr_name, (const int8_t* )usr, USR_NAME_LEN);
    return 0;
}


/**
 * sys_getusr - get user name
 * @param  buf - buf to store user name
 * @return - 0 if success; -1 if fail
 */
int32_t sys_getusr(uint8_t* buf) {
    if (usr_name[0] == 0x00) // if user name buffer is empty
        return -1;
    strncpy((int8_t* )buf, (const int8_t* )usr_name, USR_NAME_LEN);
    return 0;
}


/**
 * sys_getpid - get current pid
 * @return - current pid
 */
int32_t sys_getpid(void) {
    return cur_proc_pcb->pid;
}


/**
 * sys_textcolor - set color for text mode
 * @param attr - color attribute
 * @return - 0
 */
int32_t sys_textcolor(uint8_t attr) {
    ATTRIB = attr;
    return 0;
}


/* sys_vidmap_modex
   description: map physical video memory address to a designated one in modex mode
   input: screen_start - starting screen address
          nbytes - bytes to copy
   output: none
   return value: -1 if cannot read argument or it is invalid, 0 if success
   side effect: none
   author: Kexuan Zou
*/
int32_t sys_map_modex(uint8_t** screen_start) {
    if (screen_start == NULL) return -1; // if pointer is invalid, return
    if ((uint32_t)screen_start < USER_VIRT_TOP || (uint32_t)screen_start >= USER_VIRT_BOT)
        return -1; // if screen starting address is not within the user program's 4MB space return -1
    int i = 0;
    for (; i < MODEX_PAGE_NUM; i++)
        map_virtual_4kb_prog(VIDEO + i*MODEX_PAGE_SIZE, MODEX_VIRT_START + i*MODEX_PAGE_SIZE);
    *screen_start = (uint8_t* )MODEX_VIRT_START;
    return 0;
}

int32_t sys_getip(void) {
	return get_ip();
}
