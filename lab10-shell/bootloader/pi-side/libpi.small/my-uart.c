#include "rpi.h"
#include "uart.h"
#include "gpio.h"

#define AUX_BASE 0x20215000

static volatile unsigned *aux_mu_lsr = (volatile unsigned *)(AUX_BASE + 0x54);
static volatile unsigned *aux_mu_io = (volatile unsigned *)(AUX_BASE + 0x40);


void uart_init(void) {
	dev_barrier();
	
	gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
	gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
	
	dev_barrier();

	//broadcom page 9
	volatile unsigned *aux_enb  = (volatile unsigned *)(AUX_BASE + 0x04);
	int x = get32(aux_enb);
	x |= 1;
	put32(aux_enb, x);
	
	dev_barrier();

	//broadcom page 16
	volatile unsigned *aux_mu_cntl = (volatile unsigned *)(AUX_BASE + 0x60);
	put32(aux_mu_cntl, 0);

	//broadcom page 19
	volatile unsigned *aux_baud = (volatile unsigned *)(AUX_BASE + 0x68);
	put32(aux_baud, 270);

	//broadcom page 14
	volatile unsigned *aux_mu_lcr = (volatile unsigned *)(AUX_BASE + 0x4c);
	put32(aux_mu_lcr, 3);

	//broadcom page 13
	volatile unsigned *aux_mu_iir = (volatile unsigned *)(AUX_BASE + 0x48);
	put32(aux_mu_iir, 6);

	//broadcom page 16
	put32(aux_mu_cntl, 3);

	dev_barrier();

}


//page 15 defines the dynamics below
int uart_getc(void) {
	while((get32(aux_mu_lsr) & 1) == 1);
	return get32(aux_mu_io) & 0xFF;
}
void uart_putc(unsigned c) {
	while((get32(aux_mu_lsr) & (1 << 5)) == 0);
	put32(aux_mu_io + 8, c & 0xFF);
}
