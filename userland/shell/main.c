/*
 * ===========================================================================
 * userland/shell/main.c
 * ===========================================================================
 *
 * NexaShell - NexaKernel Command Line Interface
 *
 * This file implements a basic interactive shell for NexaKernel. It provides:
 * - Command line input with prompt
 * - Built-in commands (help, clear, ps, mem, ls, cat, echo, exit)
 * - Command parsing and execution
 * - Error handling for invalid commands
 *
 * The shell uses system calls to interact with the kernel and provides
 * a user-friendly interface for exploring the operating system.
 *
 * ===========================================================================
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * System Call Numbers (must match kernel/syscall.c)
 * --------------------------------------------------------------------------- */
#define SYS_EXIT        1
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_GETPID      20
#define SYS_YIELD       158

/* Standard file descriptors */
#define STDIN   0
#define STDOUT  1
#define STDERR  2

/* Shell constants */
#define MAX_INPUT_SIZE      256
#define MAX_ARGS            16
#define PROMPT              "NexaKernel> "

/* ---------------------------------------------------------------------------
 * System Call Wrappers (inline to avoid linking issues)
 * --------------------------------------------------------------------------- */

static inline int syscall0(int num)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num)
        : "memory"
    );
    return result;
}

static inline int syscall1(int num, int arg1)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num), "b" (arg1)
        : "memory"
    );
    return result;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3)
{
    int result;
    __asm__ volatile (
        "int $0x80"
        : "=a" (result)
        : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3)
        : "memory"
    );
    return result;
}

/* Wrapper functions */
static void shell_exit(int status)
{
    syscall1(SYS_EXIT, status);
    while (1) { }  /* Never returns */
}

static int shell_read(int fd, void *buf, size_t count)
{
    return syscall3(SYS_READ, fd, (int)buf, (int)count);
}

static int shell_write(int fd, const void *buf, size_t count)
{
    return syscall3(SYS_WRITE, fd, (int)buf, (int)count);
}

static int shell_getpid(void)
{
    return syscall0(SYS_GETPID);
}

/* ---------------------------------------------------------------------------
 * String Utilities
 * --------------------------------------------------------------------------- */

static size_t str_len(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

static int str_cmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static int str_ncmp(const char *s1, const char *s2, size_t n)
{
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static char *str_cpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* ---------------------------------------------------------------------------
 * Output Utilities
 * --------------------------------------------------------------------------- */

static void print(const char *str)
{
    shell_write(STDOUT, str, str_len(str));
}

static void println(const char *str)
{
    print(str);
    print("\n");
}

static void print_char(char c)
{
    shell_write(STDOUT, &c, 1);
}

static void print_number(int num)
{
    char buf[16];
    int i = 0;
    bool negative = false;
    
    if (num < 0) {
        negative = true;
        num = -num;
    }
    
    if (num == 0) {
        print("0");
        return;
    }
    
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (negative) {
        print_char('-');
    }
    
    while (i > 0) {
        print_char(buf[--i]);
    }
}

static void print_hex(uint32_t num)
{
    const char hex_chars[] = "0123456789ABCDEF";
    char buf[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buf[i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    print(buf);
}

/* ---------------------------------------------------------------------------
 * Input Handling
 * --------------------------------------------------------------------------- */

static int read_line(char *buffer, size_t max_size)
{
    size_t pos = 0;
    char c;
    
    while (pos < max_size - 1) {
        if (shell_read(STDIN, &c, 1) <= 0) {
            continue;
        }
        
        /* Handle backspace */
        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                /* Erase character on screen: backspace, space, backspace */
                print("\b \b");
            }
            continue;
        }
        
        /* Handle Enter key */
        if (c == '\n' || c == '\r') {
            print("\n");
            buffer[pos] = '\0';
            return (int)pos;
        }
        
        /* Handle regular characters */
        if (c >= 32 && c < 127) {
            buffer[pos++] = c;
            print_char(c);
        }
    }
    
    buffer[pos] = '\0';
    return (int)pos;
}

/* ---------------------------------------------------------------------------
 * Command Parsing
 * --------------------------------------------------------------------------- */

static int parse_args(char *input, char **argv, int max_args)
{
    int argc = 0;
    char *p = input;
    
    while (*p && argc < max_args) {
        /* Skip leading whitespace */
        while (*p && is_whitespace(*p)) {
            p++;
        }
        
        if (*p == '\0') {
            break;
        }
        
        /* Start of argument */
        argv[argc++] = p;
        
        /* Find end of argument */
        while (*p && !is_whitespace(*p)) {
            p++;
        }
        
        /* Null-terminate argument */
        if (*p) {
            *p++ = '\0';
        }
    }
    
    return argc;
}

/* ---------------------------------------------------------------------------
 * Built-in Commands
 * --------------------------------------------------------------------------- */

/* help - Display available commands */
static void cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("");
    println("========================================");
    println("  NexaKernel Shell - Built-in Commands");
    println("========================================");
    println("");
    println("  help           - Display this help message");
    println("  clear          - Clear the screen");
    println("  echo [text]    - Print text to the screen");
    println("  ps             - List running processes");
    println("  mem            - Display memory statistics");
    println("  ls             - List files in current directory");
    println("  cat [file]     - Display file contents");
    println("  pwd            - Print current directory");
    println("  whoami         - Print current user");
    println("  version        - Display kernel version");
    println("  uptime         - Display system uptime");
    println("  exit           - Exit the shell");
    println("");
}

/* clear - Clear the screen */
static void cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    /* ANSI escape sequence to clear screen and move cursor to home */
    /* Note: This requires VGA driver to support escape sequences */
    /* For now, just print newlines */
    for (int i = 0; i < 25; i++) {
        println("");
    }
    
    /* Alternative: Use a syscall to clear screen */
    /* TODO: Add SYS_CLEAR syscall */
}

/* echo - Print text */
static void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            print(" ");
        }
        print(argv[i]);
    }
    println("");
}

/* ps - List running processes */
static void cmd_ps(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("");
    println("  PID   STATE       NAME");
    println("  ---   -----       ----");
    
    /* Get our own PID */
    int pid = shell_getpid();
    print("  ");
    print_number(pid);
    println("     RUNNING     shell");
    
    /* TODO: Add syscall to enumerate all processes */
    /* For now, just show shell process */
    println("");
    println("  (Full process listing requires SYS_PS syscall)");
    println("");
}

/* mem - Display memory statistics */
static void cmd_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("");
    println("  Memory Statistics");
    println("  -----------------");
    println("");
    
    /* TODO: Add syscall to get memory info */
    println("  Total RAM:        4 MB (configured)");
    println("  Frame Size:       4 KB");
    println("  RAMFS Max Size:   4 MB");
    println("");
    println("  (Detailed stats require SYS_MEMINFO syscall)");
    println("");
}

/* ls - List files */
static void cmd_ls(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("");
    println("  Directory listing: /");
    println("  -------------------");
    println("");
    
    /* TODO: Add syscall to list directory */
    println("  (File listing requires SYS_READDIR syscall)");
    println("  (RAMFS is initialized but no files created yet)");
    println("");
}

/* cat - Display file contents */
static void cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        println("Usage: cat <filename>");
        return;
    }
    
    const char *filename = argv[1];
    
    println("");
    print("  Contents of: ");
    println(filename);
    println("  ---------------");
    println("");
    
    /* TODO: Use open/read/close syscalls */
    println("  (File reading requires SYS_OPEN/SYS_READ syscalls)");
    println("");
}

/* pwd - Print working directory */
static void cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("/");
}

/* whoami - Print current user */
static void cmd_whoami(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("root");
}

/* version - Display kernel version */
static void cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("");
    println("  NexaKernel v0.1.0");
    println("  -----------------");
    println("  An educational x86 operating system kernel");
    println("  with DSA-driven subsystems");
    println("");
    println("  Architecture: x86 (i386)");
    println("  Mode: Protected Mode (32-bit)");
    println("");
}

/* uptime - Display system uptime */
static void cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    /* TODO: Get tick count from kernel */
    println("");
    println("  System uptime: (requires SYS_UPTIME syscall)");
    println("");
}

/* exit - Exit the shell */
static void cmd_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    println("Goodbye!");
    shell_exit(0);
}

/* ---------------------------------------------------------------------------
 * Command Table
 * --------------------------------------------------------------------------- */

typedef struct {
    const char *name;
    void (*handler)(int argc, char **argv);
    const char *description;
} command_t;

static const command_t commands[] = {
    { "help",    cmd_help,    "Display help message" },
    { "clear",   cmd_clear,   "Clear the screen" },
    { "echo",    cmd_echo,    "Print text" },
    { "ps",      cmd_ps,      "List processes" },
    { "mem",     cmd_mem,     "Memory statistics" },
    { "ls",      cmd_ls,      "List files" },
    { "cat",     cmd_cat,     "Display file contents" },
    { "pwd",     cmd_pwd,     "Print current directory" },
    { "whoami",  cmd_whoami,  "Print current user" },
    { "version", cmd_version, "Display kernel version" },
    { "uptime",  cmd_uptime,  "Display system uptime" },
    { "exit",    cmd_exit,    "Exit the shell" },
    { NULL, NULL, NULL }  /* Sentinel */
};

/* ---------------------------------------------------------------------------
 * Command Execution
 * --------------------------------------------------------------------------- */

static bool execute_command(int argc, char **argv)
{
    if (argc == 0) {
        return true;  /* Empty command, just show prompt again */
    }
    
    const char *cmd_name = argv[0];
    
    /* Search command table */
    for (int i = 0; commands[i].name != NULL; i++) {
        if (str_cmp(cmd_name, commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return true;
        }
    }
    
    /* Command not found */
    print("Unknown command: ");
    println(cmd_name);
    println("Type 'help' for available commands.");
    
    return false;
}

/* ---------------------------------------------------------------------------
 * Shell Banner
 * --------------------------------------------------------------------------- */

static void print_banner(void)
{
    println("");
    println("  =============================================");
    println("  |                                           |");
    println("  |     _   _                 _  __           |");
    println("  |    | \\ | | _____  ____ _| |/ /___ _ __   |");
    println("  |    |  \\| |/ _ \\ \\/ / _` | ' // _ \\ '__|  |");
    println("  |    | |\\  |  __/>  < (_| | . \\  __/ |     |");
    println("  |    |_| \\_|\\___/_/\\_\\__,_|_|\\_\\___|_|     |");
    println("  |                                           |");
    println("  =============================================");
    println("");
    println("  Welcome to NexaKernel Shell v0.1.0");
    println("  Type 'help' for available commands.");
    println("");
}

/* ---------------------------------------------------------------------------
 * Main Entry Point
 * --------------------------------------------------------------------------- */

int main(void)
{
    char input_buffer[MAX_INPUT_SIZE];
    char *argv[MAX_ARGS];
    int argc;
    
    /* Print welcome banner */
    print_banner();
    
    /* Main shell loop */
    while (1) {
        /* Print prompt */
        print(PROMPT);
        
        /* Read input line */
        int len = read_line(input_buffer, MAX_INPUT_SIZE);
        
        /* Skip empty lines */
        if (len == 0) {
            continue;
        }
        
        /* Parse command line into arguments */
        argc = parse_args(input_buffer, argv, MAX_ARGS);
        
        /* Execute command */
        execute_command(argc, argv);
    }
    
    return 0;
}
