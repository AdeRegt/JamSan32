#define TRUE 1
#define FALSE 0
#define NULL 0
#define MAX_WIDTH_SCREEN 80
#define IDT_SIZE 256


// <editor-fold defaultstate="collapsed" desc="to resolve the ... part">
// http://f.osdev.org/viewtopic.php?f=1&t=7415
#define stdarg_hxx

typedef void * va_list;

#define __va_size( type ) \
( ( sizeof( type ) + 3 ) & ~0x3 )

#define va_start( va_l, last ) \
( ( va_l ) = ( void * )&( last ) + __va_size( last ) )

#define va_end( va_l )

#define va_arg( va_l, type ) \
( ( va_l ) += __va_size( type ), \
*( ( type * )( ( va_l ) - __va_size( type ) ) ) )
// </editor-fold>


void kernel_main();
int printf(const char* msg,...);
void putc(char e);
void hang();
void hlt();
int strlen(const char* a);
char* itoa( int value, char* str, int base );
int print(const char* msg);
void lidt();
void setInterrupt(int i,unsigned long loc);
void defaulte();
extern void irq_defaulte();
extern void irq_error();
extern void irq_hdd();
unsigned char inportb (unsigned short _port);
unsigned short inportw (unsigned short _port);
void outportb (unsigned short _port, unsigned char _data);
void outportw (unsigned short _port, unsigned short _data);
void memcopy(char *a, char *b,int c);
void error(char* msg);

char *videomemory = (char*) 0xb8000;
int videomemorypointer = 0;
int curX = 0;
int curY = 0;
char kleur = 0x07;

typedef struct {
    char exists; // 0 = no | 1 = yes
    unsigned long read; // read(location,lba)
}device;

typedef struct {
    char exists;
    char driveletter;
    device dev;
    unsigned long readFile; // read(location,filename,dev)
    unsigned long readDIR; // read(location,filename,dev)
}filesystem;

device devices[10];
filesystem filesystems[5];
int devicescount = 0;
unsigned char *loc = (unsigned char*) 0x1000;
void readHDDLBA(unsigned char* location, unsigned long lba);
void readCDLBA(unsigned char* location, unsigned long LBA);
char* readSFSRootDir(device dev);
void readSFSFile(const char* name,device dev);
void fopen(char* path);

/**
 * Main routine for our kernel
 */
void kernel_main(){
    curY = 12;
    curX = 13;
    printf("Kernel created by James Rumbal and Alexandros de Regt\n");
    lidt();
    devices[devicescount].exists        = TRUE;
    devices[devicescount].read          = (unsigned long)&readHDDLBA;
    ((void(*)())devices[devicescount].read)(loc,0);
    filesystems[0].dev                  = devices[devicescount];
    filesystems[0].driveletter          = 'C';
    filesystems[0].exists               = TRUE;
    filesystems[0].readDIR              = (unsigned long)&readSFSRootDir;
    filesystems[0].readFile             = (unsigned long)&readSFSFile;
    printf("The following devices are there:\n");
    int i = 0;
    for(i = 0 ; i < 5 ; i++){
        if(filesystems[i].exists){
            printf("-Driver %c\n",filesystems[i].driveletter);
        }
    }
    printf("End of devices dump\n");
    fopen("C@opteas721112");
    hang();
}

// HDD
// <editor-fold defaultstate="collapsed" desc="General fs">

/**
 * Opens a file at loc
 * @param path the complete path to the file
 */
void fopen(char* path) {
    char driveletter = path[0];
    int i = 0;
    for (i = 0; i < 5; i++) {
        if (filesystems[i].driveletter == driveletter) {
            goto cont;
        }
    }
    error("Invalid driveletter");
cont:
    path++;
    path++;
    ((void(*)())filesystems[i].readFile)(path, filesystems[i].dev);
}// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="HDD">
char hddintfired = 0x00;

/**
 * The HDD interrupt
 */
void hdd() {
    hddintfired = 1;
}

/**
 * Reads a file on the SFS filesystem
 * @param name path of the file
 * @param dev from which device?
 */
void readSFSFile(const char* name, device dev) {
    ((void(*)())dev.read)(loc, 1);
    int bufferdir = 0;
    for (bufferdir = 0; bufferdir < 512; bufferdir++) {
        int i = 0;
        for (i = 0; i < 12; i++) {
            char deze = loc[bufferdir++];
            char doze = name[i];
            if (deze != doze) {
                break;
            }
        }
        if (i == 12) {
            unsigned long int l = loc[bufferdir + 0] | (loc[bufferdir + 1] << 8) | (loc[bufferdir + 2] << 16) | (loc[bufferdir + 3] << 24);
            ((void(*)())dev.read)(loc, l);
            return;
        }
        bufferdir = bufferdir + 4;
    }
    error("File not found");
}

/**
 * Reads the mainfilesystem at the disk
 * @param dev which disk
 * @return the filesystemstring
 */
char* readSFSRootDir(device dev) {
    ((void(*)())dev.read)(loc, 1);
    char* message = "                                                                                                                                                                                                                                                                                                ";
    int messagedir = 0;
    int bufferdir = 0;
    for (bufferdir = 0; bufferdir < 512; bufferdir++) {
        int i = 0;
        for (i = 0; i < 12; i++) {
            char deze = loc[bufferdir++];
            if (deze == 0x00) {
                goto clean;
            }
            message[messagedir++] = deze;
        }
        message[messagedir++] = ',';
        bufferdir++;
        bufferdir++;
        bufferdir++;
        bufferdir++;
    }
clean:
    message[messagedir] = 0x00;
    return message;
}

/**
 * Reads the HDD in LBA mode
 * @param location where to store the buffer
 * @param LBA which part to load
 */
void readHDDLBA(unsigned char* location, unsigned long LBA) {
    //printf("Reading hdd at LBA %i ",LBA);
    outportb(0x1F6, 0xE0);
    outportb(0x1F1, NULL);
    outportb(0x1F2, 1); // count
    outportb(0x1F3, (unsigned char) LBA);
    outportb(0x1F4, (unsigned char) (LBA >> 8));
    outportb(0x1F5, (unsigned char) (LBA >> 16));
    hddintfired = 0x00;
    outportb(0x1F7, 0x20);
    //int pop1 = 0;
    //int pop2 = 0;
    while (TRUE) {
        char val = inportb(0x1F7);
        if ((val >> 0) & 1) {
            char val = inportb(0x1F7);
            if (val & 0x80) {
                error("HDD: Bad sector");
            } else if (val & 0x40) {
                error("HDD: Uncorrectable data");
            } else if (val & 0x20) {
                error("HDD: No media");
            } else if (val & 0x10) {
                error("HDD: ID mark not found");
            } else if (val & 0x08) {
                error("HDD: No media");
            } else if (val & 0x04) {
                error("HDD: Command aborted");
            } else if (val & 0x02) {
                error("HDD: Track 0 not found");
            } else if (val & 0x01) {
                error("HDD: No address mark");
            } else {
                error("HDD error bit 0 is set");
            }
        }
        if ((val >> 3) & 1) {
            break;
        }
    }
    int i = 0;
    int z = 0;
    for (i = 0; i < (512 / 2); i++) {
        short d = inportw(0x1F0);
        location[z++] = d;
        location[z++] = (unsigned char) (d >> 8);
    }
}
// END HDD// </editor-fold>

// TIMER
 
// END TIMER

// <editor-fold defaultstate="collapsed" desc="Hardware abstraction Layer HAL">

void memcopy(char *a, char *b,int c){
    int i = 0;
    for(i = 0 ; i < c ; i++){
        b[i] = a[i];
    }
}
/**
 * http://www.osdever.net/bkerndev/Docs/whatsleft.htm
 * Get data from port
 * @param _port which port
 * @return data
 */
unsigned char inportb(unsigned short _port) {
    unsigned char rv;
    __asm__ __volatile__("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

unsigned short inportw(unsigned short _port) {
    unsigned short rv;
    __asm__ __volatile__("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

/**
 * http://www.osdever.net/bkerndev/Docs/whatsleft.htm
 * Send data to port
 * @param _port which port
 * @param _data data
 */
void outportb(unsigned short _port, unsigned char _data) {
    __asm__ __volatile__("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void outportw(unsigned short _port, unsigned short _data) {
    __asm__ __volatile__("outw %1, %0" : : "dN" (_port), "a" (_data));
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="IDT">

/* Defines an IDT entry */
struct idt_entry {
    unsigned short base_lo;
    unsigned short sel; /* Our kernel segment goes here! */
    unsigned char always0; /* This will ALWAYS be set to 0! */
    unsigned char flags; /* Set using the above table! */
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_entry idt[IDT_SIZE];
struct idt_ptr idtp;

/**
 * Let the CPU know where the idt is
 */
void lidt() {
    // setup PIC http://www.osdever.net/bkerndev/Docs/whatsleft.htm
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xA1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
    // set all ints
    int i = 0;
    for (i = 0; i < IDT_SIZE; i++) {
        setInterrupt(i, (unsigned long) &irq_defaulte);
    }
    for(i = 0 ; i < 10 ; i++){
        setInterrupt(i, (unsigned long) &irq_error);
    }
    //for(i = 30 ; i < 33 ; i++){//35
    //setInterrupt(32, (unsigned long) &irq_timer);
    //}
    setInterrupt(32+14, (unsigned long) &irq_hdd);
    idtp.limit = (sizeof (struct idt_entry) * IDT_SIZE) - 1;
    idtp.base = (unsigned int) &idt;
    asm volatile("lidt idtp\nsti");
    //idt_load();
}

/**
 * The default int shows a dot on the screen
 */
void defaulte() {
    
}

/**
 * Errormessage
 */
void error(char* msg) {
    curX = 0;
    curY = 0;
    kleur = 0x70;
    printf("Unhandeld Exception: %s",msg);
    asm volatile("cli\nhlt");
}

/**
 * Set interrupt
 * @param i interrupt number
 * @param base the function to go to
 */
void setInterrupt(int i, unsigned long base) {
    idt[i].base_lo = (base & 0xFFFF);
    idt[i].base_hi = (base >> 16) & 0xFFFF;
    idt[i].sel = 0x10;
    idt[i].always0 = 0;
    idt[i].flags = 0x8E;
}// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="String operations">

/**
 * Prints a string on the screen at curX*curY
 * Supported:
 * %s = string
 * %c = char
 * %i = int
 * @param msg format message
 * @param ... other components
 * @return length of the string
 */
int printf(const char* msg, ...) {
    va_list parameters;
    va_start(parameters, msg);
    int i = 0; // size of the string
    while (TRUE) {
        char deze = msg[i];
        if (deze == NULL) { // check if this is the end of the string
            break;
        }
        i++;
        if (deze == '%') { // extra thing
            deze = msg[i];
            i++;
            if (deze == 's') { // character is an string
                const char* mms = (char*) va_arg(parameters, const char*);
                print(mms);
            } else if (deze == 'c') {
                char c = (char) va_arg(parameters, int);
                putc(c);
            } else if (deze == 'i') {
                int c = (int) va_arg(parameters, int);
                char* ss = "     ";
                itoa(c + 1000, ss, 10);
                print(ss);
            } else {
                print("< SYNTAX ERROR >");
            }
        } else { // just a character
            putc(deze);
        }
    }
    return i;
}

/**
 * The length of an string
 * @param a the string
 * @return the size
 */
int strlen(const char* a) {
    int i = 0;
    while (TRUE) {
        if (a[i++] == NULL) {
            break;
        }
    }
    return i;
}

/**
 * Puts a character at curX*curY
 * @param e the character to print
 */
void putc(char deze) {
    // calculates the videomemorypointer
    videomemorypointer = (curY * (MAX_WIDTH_SCREEN * 2))+(curX * 2);
    if (deze == '\n') { // new line
        curY++;
        curX = 0;
    } else {
        videomemory[videomemorypointer++] = deze;
        videomemory[videomemorypointer++] = kleur; // color of the string
        curX++;
    }
}

/**
 * Starts an endless loop
 */
void hang() {
    for (;;);
}

/**
 * Stops the processor
 */
void hlt() {
    asm volatile("hlt");
}

/**
 * Int to char
 * http://wiki.osdev.org/Printing_To_Screen
 * @param value thing
 * @param str string to 
 * @param base 10 base
 * @return 
 */
char* itoa(int value, char* str, int base) {
    char * rc;
    char * ptr;
    char * low;
    // Check for supported base.
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals.
    if (value < 0 && base == 10) {
        *ptr++ = '-';
    }
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do {
        // Modulo is negative for negative value. This trick makes abs() unnecessary.
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while (value);
    // Terminating the string.
    *ptr-- = '\0';
    // Invert the numbers.
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

/**
 * Prints a non formatted string
 * @param msg non formated string
 * @return size of string
 */
int print(const char* msg) {
    int i = 0; // size of the string
    while (TRUE) {
        char deze = msg[i];
        if (deze == NULL) { // check if this is the end of the string
            break;
        }
        i++;
        putc(deze);
    }
    return i;
}// </editor-fold>
