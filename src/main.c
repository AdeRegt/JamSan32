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
unsigned char inportb (unsigned short _port);
void outportb (unsigned short _port, unsigned char _data);

char *videomemory = (char*) 0xb8000;
int videomemorypointer = 0;
int curX = 0;
int curY = 0;
char kleur = 0x07;



/**
 * Main routine for our kernel
 */
void kernel_main(){
    curY = 12;
    curX = 13;
    printf("Kernel created by James Rumbal and Alexandros de Regt\n");
    lidt();
    hang();
}

/**
 * http://www.osdever.net/bkerndev/Docs/whatsleft.htm
 * Get data from port
 * @param _port which port
 * @return data
 */
unsigned char inportb (unsigned short _port){
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

/**
 * http://www.osdever.net/bkerndev/Docs/whatsleft.htm
 * Send data to port
 * @param _port which port
 * @param _data data
 */
void outportb (unsigned short _port, unsigned char _data){
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

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

struct regs{
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;    
};

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
void error() {
    curX = 0;
    curY = 0;
    kleur = 0x70;
    printf("Unhandeld Exception: System halted!");
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
