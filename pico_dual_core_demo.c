#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static bool memory_clobbering = false;

// Silly little function to print hex characters without
// any fancy math beyond shifts.
static void print_hex(uart_inst_t * uart_id, uint32_t hex) {
    const char digits[] = "0123456789abcdef";
    int offset;
    bool seen = false;

    for (offset = 7; offset >= 0; offset -= 1) {
        int digit = ((hex&(0xff<<(offset*4)))>>(offset*4))&0xf;
        if ((digit == 0) && (offset == 0)) {
            uart_putc(uart_id, '0');
            continue;
        }
        if (((!seen) && (digit == 0))) {
            continue;
        }
        seen = true;
        uart_putc(uart_id, digits[digit]);
    }
}

// Place this array in the ram vector table area,
// which ends up at 0x20000000. This will get scribbled over
// by the `clobber_memory()` function, but that should be
// fine since we don't actually use any interrupts.
__attribute__((used, section(".ram_vector_table")))
uint8_t DATA[256];
static void clobber_memory(void) {
    memory_clobbering = true;
    uint8_t data_offset = 0;
    while (1) {
        DATA[data_offset++]++;
    }
}

// Constantly print out a number so we know the target is alive.
static void print_loop(void) {
    int i = 0;

    while (1) {
        uart_puts(UART_ID, "Loop counter!  i: 0x");
        print_hex(UART_ID, i);
        uart_puts(UART_ID, "  Core: ");
        uart_putc(UART_ID, '0' + *((volatile uint32_t *)SIO_CPUID_OFFSET));
        if (memory_clobbering) {
            uart_puts(UART_ID, "  Memory is being clobbered");
        }
        uart_puts(UART_ID, "\r\n");
        i += 1;
    }
}

void main(void) {
    // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Comment out this line to get things working
    multicore_launch_core1(clobber_memory);

    print_loop();
}