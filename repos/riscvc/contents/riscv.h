#pragma once

#include <cstdint>
#include <cstddef>

// Instruction shape

// Bits filled out from diagram in riscv-spec unprivileged, 20190621-draft, sec 2.3

// All instructions
#define OPCODE    0
#define OPCODE_TO 6

// R, I, U, J
#define RD     7
#define RD_TO 11

// R, I, S, B
#define FUNCT3    12
#define FUNCT3_TO 14
#define RS1    15
#define RS1_TO 19

// R, S, B
#define RS2    20
#define RS2_TO 24

// R
#define FUNCT7    25
#define FUNCT7_TO 31

// I

#define IMMI    20
#define IMMI_TO 24

// SYSTEM (I)

#define FUNCT12    20
#define FUNCT12_TO 24

// S

#define IMMS1     7
#define IMMS1_TO 11
#define IMMS2    25
#define IMMS2_TO 31

// B

#define IMMB1     8
#define IMMB1_TO 11
#define IMMB2    25
#define IMMB2_TO 30
#define IMMB3    7
#define IMMB3_TO 7
#define IMMB4    31
#define IMMB4_TO 31

// U

#define IMMU    12
#define IMMU_TO 31

// J

#define IMMJ1    21
#define IMMJ1_TO 30 
#define IMMJ2    20
#define IMMJ2_TO 20 
#define IMMJ3    12
#define IMMJ3_TO 19 
#define IMMJ4    31
#define IMMJ4_TO 31 

#define MASK(V) ( (uint32_t)( (V) == 31 ? ~((uint32_t)0) : ( (V) == 0 ? 0 : (1<<(V))-1 ) ) )
#define VREAD(VAR, FIELD) ( ((VAR) & ( MASK(FIELD##_TO) & ~MASK(FIELD) ) ) >> FIELD )
#define VWRIT(VAR, FIELD, VALUE) VAR = ( ((VALUE) & MASK(FIELD##_TO - FIELD)) << FIELD )

// Emulator state

struct Emulator {
	uint32_t reg[32]; // 0 is pc
	uint8_t *memory;
	size_t memoryLen;

	Emulator(uint8_t _memoryLen);
	void run(uint32_t instr);
};

// Opcodes

#include "opcode.h"
