/* 
 * very simple bootloader.  more robust than xmodem.   (that code seems to 
 * have bugs in terms of recovery with inopportune timeouts.)
 */

#define __SIMPLE_IMPL__
#include "../shared-code/simple-boot.h"

#include "libpi.small/rpi.h"

static void send_byte(unsigned char uc) {
	uart_putc(uc);
}
static unsigned char get_byte(void) { 
        return uart_getc();
}

static unsigned get_uint(void) {
	unsigned u = get_byte();
        u |= get_byte() << 8;
        u |= get_byte() << 16;
        u |= get_byte() << 24;
	return u;
}
static void put_uint(unsigned u) {
        send_byte((u >> 0)  & 0xff);
        send_byte((u >> 8)  & 0xff);
        send_byte((u >> 16) & 0xff);
        send_byte((u >> 24) & 0xff);
}

static void die(int code) {
        put_uint(code);
        reboot();
}

//  bootloader:
//	1. wait for SOH, size, cksum from unix side.
//	2. echo SOH, checksum(size), cksum back.
// 	3. wait for ACK.
//	4. read the bytes, one at a time, copy them to ARMBASE.
//	5. verify checksum.
//	6. send ACK back.
//	7. wait 500ms 
//	8. jump to ARMBASE.
//
void notmain(void) {

	uart_init();
	delay_ms(500);
    while (get_uint() != SOH);

	unsigned size = get_uint();
	unsigned crc32_value = get_uint();

	put_uint(SOH);
	put_uint(size);
	put_uint(crc32_value);
    int ptr = 0;
    for (; ptr < size / 4; ptr += 1) {
        unsigned* addr = (unsigned *) ARMBASE + ptr;
        addr = get_uint();
    }
	if( get_uint() != EOT) put_uint(BAD_END);
	else put_uint(EOT);
	
//	gpio_set_io_off(20);
	
	if(crc32((unsigned *)ARMBASE, size/4) != crc32_value){
		put_uint(BAD_CKSUM);
	}
	else{
		put_uint(ACK);
	}

	delay_ms(500);

	// run what client sent.
        BRANCHTO(ARMBASE);
	// should not get back here, but just in case.
	reboot();
}
