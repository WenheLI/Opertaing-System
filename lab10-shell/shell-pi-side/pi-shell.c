#include "rpi.h"
#include "pi-shell.h"

// read characters until we hit a newline.
static int readline(char *buf, int sz) {
	for(int i = 0; i < sz; i++) {
		if((buf[i] = uart_getc()) == '\n') {
			buf[i] = 0;
			return i;
		}
	}
}

void notmain() { 
	uart_init();
	int n;
	char buf[1024];

	delay_ms(100);
	printk("Welcome !!! \n >");
	while((n = readline(buf, sizeof buf))) {	
			if(strncmp(buf, "echo", 4) == 0) {
				printk("PI: > %s\n", buf + 4);
			} else if (strncmp(buf, "reboot", 6) == 0) {
				printk("PI: > PI REBOOT!!!\n");
				delay_ms(100);
				rpi_reboot();
			} else {
				printk("PI: > \n");
			}
		}
	clean_reboot();
}
