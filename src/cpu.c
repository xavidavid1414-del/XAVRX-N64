#include "xavrx/cpu.h"

#include <string.h>

#define OP(i) ((uint32_t)((i) >> 26))
#define RS(i) (((i) >> 21) & 31u)
#define RT(i) (((i) >> 16) & 31u)
#define RD(i) (((i) >> 11) & 31u)
#define SA(i) (((i) >> 6) & 31u)
#define FN(i) ((i) & 63u)
#define IMM(i) ((uint16_t)((i) & 0xffffu))
#define TARGET(i) ((i) & 0x03ffffffu)

static uint64_t sign_extend32(uint32_t value) {
    return (uint64_t)(int64_t)(int32_t)value;
}

static bool translate_address(uint64_t virtual_address, uint32_t *physical) {
    uint32_t address = (uint32_t)virtual_address;
    if ((virtual_address >> 32) != 0u && (virtual_address >> 32) != 0xffffffffu) return false;
    if (address < 0x20000000u) {
        *physical = address;
        return true;
    }
    if ((address >= 0x80000000u && address < 0xa0000000u) ||
        (address >= 0xa0000000u && address < 0xc0000000u)) {
        *physical = address & 0x1fffffffu;
        return true;
    }
    return false; /* TLB-mapped regions are implemented in a later increment. */
}

void xavrx_cpu_reset(xavrx_cpu_t *cpu, uint64_t reset_vector) {
    if (cpu == NULL) return;
    memset(cpu, 0, sizeof(*cpu));
    cpu->pc = reset_vector;
    cpu->next_pc = reset_vector + 4u;
    cpu->gpr[0] = 0u;
}

static xavrx_cpu_result_t exception(xavrx_cpu_t *cpu, uint32_t code) {
    cpu->cp0[13] = ((uint64_t)code << 2);
    cpu->cp0[14] = cpu->pc;
    cpu->pc = 0xffffffff80000180ull;
    cpu->next_pc = cpu->pc + 4u;
    return XAVRX_CPU_EXCEPTION;
}

xavrx_cpu_result_t xavrx_cpu_step(xavrx_cpu_t *cpu, xavrx_bus_t *bus) {
    uint32_t physical, instruction;
    uint64_t current_pc, following_pc, a, b, result = 0u;
    uint32_t op, rs, rt, rd, shift, function;
    int64_t signed_a, signed_b;
    int32_t immediate;

    if (cpu == NULL || bus == NULL || cpu->halted) return XAVRX_CPU_BUS_ERROR;
    current_pc = cpu->pc;
    following_pc = cpu->next_pc;
    if ((current_pc & 3u) != 0u || !translate_address(current_pc, &physical) ||
        !xavrx_bus_read32(bus, physical, &instruction)) return XAVRX_CPU_BUS_ERROR;

    cpu->pc = following_pc;
    cpu->next_pc = following_pc + 4u;
    op = OP(instruction);
    rs = RS(instruction);
    rt = RT(instruction);
    rd = RD(instruction);
    shift = SA(instruction);
    function = FN(instruction);
    a = cpu->gpr[rs];
    b = cpu->gpr[rt];
    signed_a = (int64_t)a;
    signed_b = (int64_t)b;
    immediate = (int16_t)IMM(instruction);

    if (op == 0u) {
        switch (function) {
            case 0x00: result = (uint32_t)b << shift; result = sign_extend32((uint32_t)result); break;
            case 0x02: result = sign_extend32((uint32_t)b >> shift); break;
            case 0x03: result = sign_extend32((uint32_t)((int32_t)b >> shift)); break;
            case 0x08: cpu->next_pc = a; break; /* JR */
            case 0x09: result = cpu->pc + 4u; cpu->next_pc = a; if (rd != 0u) cpu->gpr[rd] = result; break;
            case 0x14: result = b << (a & 63u); break;
            case 0x16: result = b >> (a & 63u); break;
            case 0x17: result = (uint64_t)((int64_t)b >> (a & 63u)); break;
            case 0x18: {
                int64_t product = (int64_t)(int32_t)a * (int64_t)(int32_t)b;
                cpu->lo = sign_extend32((uint32_t)product);
                cpu->hi = sign_extend32((uint32_t)((uint64_t)product >> 32));
                break;
            }
            case 0x19: {
                uint64_t product = (uint64_t)(uint32_t)a * (uint64_t)(uint32_t)b;
                cpu->lo = sign_extend32((uint32_t)product);
                cpu->hi = sign_extend32((uint32_t)(product >> 32));
                break;
            }
            case 0x1a:
                if ((uint32_t)b != 0u) {
                    int32_t dividend = (int32_t)a, divisor = (int32_t)b;
                    if (dividend == INT32_MIN && divisor == -1) {
                        cpu->lo = sign_extend32((uint32_t)INT32_MIN);
                        cpu->hi = 0u;
                    } else {
                        cpu->lo = sign_extend32((uint32_t)(dividend / divisor));
                        cpu->hi = sign_extend32((uint32_t)(dividend % divisor));
                    }
                }
                break;
            case 0x1b:
                if ((uint32_t)b != 0u) {
                    cpu->lo = sign_extend32((uint32_t)a / (uint32_t)b);
                    cpu->hi = sign_extend32((uint32_t)a % (uint32_t)b);
                }
                break;
            case 0x2c: {
                uint64_t sum;
                if (__builtin_add_overflow(a, b, &sum)) return exception(cpu, 12u);
                result = sum;
                break;
            }
            case 0x2d: result = a + b; break;
            case 0x2e: {
                uint64_t difference;
                if (__builtin_sub_overflow(a, b, &difference)) return exception(cpu, 12u);
                result = difference;
                break;
            }
            case 0x2f: result = a - b; break;
            case 0x38: result = b << shift; break;
            case 0x3a: result = b >> shift; break;
            case 0x3b: result = (uint64_t)((int64_t)b >> shift); break;
            case 0x3c: result = b << (shift + 32u); break;
            case 0x3e: result = b >> (shift + 32u); break;
            case 0x3f: result = (uint64_t)((int64_t)b >> (shift + 32u)); break;
            case 0x0c: return exception(cpu, 8u); /* SYSCALL */
            case 0x0d: return exception(cpu, 9u); /* BREAK */
            case 0x20: {
                int64_t sum = (int64_t)(int32_t)a + (int64_t)(int32_t)b;
                if (sum > INT32_MAX || sum < INT32_MIN) return exception(cpu, 12u);
                result = sign_extend32((uint32_t)sum);
                break;
            }
            case 0x21: result = sign_extend32((uint32_t)((uint32_t)a + (uint32_t)b)); break;
            case 0x22: {
                int64_t difference = (int64_t)(int32_t)a - (int64_t)(int32_t)b;
                if (difference > INT32_MAX || difference < INT32_MIN) return exception(cpu, 12u);
                result = sign_extend32((uint32_t)difference);
                break;
            }
            case 0x23: result = sign_extend32((uint32_t)((uint32_t)a - (uint32_t)b)); break;
            case 0x24: result = a & b; break;
            case 0x25: result = a | b; break;
            case 0x26: result = a ^ b; break;
            case 0x27: result = ~(a | b); break;
            case 0x2a: result = signed_a < signed_b ? 1u : 0u; break;
            case 0x2b: result = a < b ? 1u : 0u; break;
            default: return exception(cpu, 10u);
        }
        if (function != 0x08u && function != 0x09u && rd != 0u) cpu->gpr[rd] = result;
    } else {
        uint64_t address;
        uint32_t data32;
        uint16_t data16;
        uint8_t data8;
        switch (op) {
            case 0x02: cpu->next_pc = (cpu->pc & 0xfffffffff0000000ull) | ((uint64_t)TARGET(instruction) << 2); break;
            case 0x03: cpu->gpr[31] = cpu->pc + 4u; cpu->next_pc = (cpu->pc & 0xfffffffff0000000ull) | ((uint64_t)TARGET(instruction) << 2); break;
            case 0x04: if (a == b) cpu->next_pc = cpu->pc + ((int64_t)immediate * 4); break;
            case 0x05: if (a != b) cpu->next_pc = cpu->pc + ((int64_t)immediate * 4); break;
            case 0x06: if ((int64_t)a <= 0) cpu->next_pc = cpu->pc + ((int64_t)immediate * 4); break;
            case 0x07: if ((int64_t)a > 0) cpu->next_pc = cpu->pc + ((int64_t)immediate * 4); break;
            case 0x18: {
                int64_t sum;
                if (__builtin_add_overflow((int64_t)a, (int64_t)immediate, &sum))
                    return exception(cpu, 12u);
                result = (uint64_t)sum;
                if (rt) cpu->gpr[rt] = result;
                break;
            }
            case 0x19: result = a + (int64_t)immediate; if (rt) cpu->gpr[rt] = result; break;
            case 0x08: {
                int64_t sum = (int64_t)(int32_t)a + (int64_t)immediate;
                if (sum > INT32_MAX || sum < INT32_MIN) return exception(cpu, 12u);
                result = sign_extend32((uint32_t)sum);
                if (rt) cpu->gpr[rt] = result;
                break;
            }
            case 0x09: result = sign_extend32((uint32_t)((uint32_t)a + (uint32_t)immediate)); if (rt) cpu->gpr[rt] = result; break;
            case 0x0a: if (rt) cpu->gpr[rt] = signed_a < (int64_t)immediate; break;
            case 0x0b: if (rt) cpu->gpr[rt] = a < (uint64_t)(int64_t)immediate; break;
            case 0x0c: if (rt) cpu->gpr[rt] = a & IMM(instruction); break;
            case 0x0d: if (rt) cpu->gpr[rt] = a | IMM(instruction); break;
            case 0x0e: if (rt) cpu->gpr[rt] = a ^ IMM(instruction); break;
            case 0x0f: if (rt) cpu->gpr[rt] = (uint64_t)(int64_t)(int32_t)((uint32_t)IMM(instruction) << 16); break;
            case 0x20: case 0x24: case 0x21: case 0x25: case 0x23: case 0x27:
            case 0x28: case 0x29: case 0x2b: case 0x37: case 0x3f: {
                address = a + (int64_t)immediate;
                if (!translate_address(address, &physical)) return XAVRX_CPU_BUS_ERROR;
                if (op == 0x20 || op == 0x24) {
                    if (!xavrx_bus_read8(bus, physical, &data8)) return XAVRX_CPU_BUS_ERROR;
                    if (rt) cpu->gpr[rt] = op == 0x20 ? sign_extend32((uint32_t)(int32_t)(int8_t)data8) : data8;
                } else if (op == 0x21 || op == 0x25) {
                    if (!xavrx_bus_read16(bus, physical, &data16)) return XAVRX_CPU_BUS_ERROR;
                    if (rt) cpu->gpr[rt] = op == 0x21 ? sign_extend32((uint32_t)(int32_t)(int16_t)data16) : data16;
                } else if (op == 0x23 || op == 0x27) {
                    if (!xavrx_bus_read32(bus, physical, &data32)) return XAVRX_CPU_BUS_ERROR;
                    if (rt) cpu->gpr[rt] = op == 0x23 ? sign_extend32(data32) : data32;
                } else if (op == 0x37) {
                    uint64_t data64;
                    if (!xavrx_bus_read64(bus, physical, &data64)) return XAVRX_CPU_BUS_ERROR;
                    if (rt) cpu->gpr[rt] = data64;
                } else if (op == 0x3f) {
                    if (!xavrx_bus_write64(bus, physical, b)) return XAVRX_CPU_BUS_ERROR;
                } else if (op == 0x28) {
                    if (!xavrx_bus_write8(bus, physical, (uint8_t)b)) return XAVRX_CPU_BUS_ERROR;
                } else if (op == 0x29) {
                    if (!xavrx_bus_write16(bus, physical, (uint16_t)b)) return XAVRX_CPU_BUS_ERROR;
                } else if (!xavrx_bus_write32(bus, physical, (uint32_t)b)) return XAVRX_CPU_BUS_ERROR;
                break;
            }
            default: return exception(cpu, 10u);
        }
    }
    cpu->gpr[0] = 0u;
    cpu->instructions_executed++;
    return XAVRX_CPU_OK;
}
