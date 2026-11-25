#ifndef XAVRX_CPU_H
#define XAVRX_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "xavrx/bus.h"

typedef enum {
    XAVRX_CPU_OK = 0,
    XAVRX_CPU_EXCEPTION = 1,
    XAVRX_CPU_BUS_ERROR = 2
} xavrx_cpu_result_t;

typedef struct {
    uint64_t gpr[32];
    uint64_t hi;
    uint64_t lo;
    uint64_t pc;
    uint64_t next_pc;
    uint64_t cp0[32];
    uint32_t instructions_executed;
    bool halted;
} xavrx_cpu_t;

void xavrx_cpu_reset(xavrx_cpu_t *cpu, uint64_t reset_vector);
xavrx_cpu_result_t xavrx_cpu_step(xavrx_cpu_t *cpu, xavrx_bus_t *bus);

#endif
