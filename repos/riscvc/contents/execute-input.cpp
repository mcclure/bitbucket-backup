//! PROLOGUE

#include <stdio.h>
#include "riscv.h"

//! END

//! METHOD
void Emulator::run(uint32_t instr) {
	int x;
//! CONTENT
	LUI:
		printf("test\n");
	default:
		printf("Bad instruction!\n");
//! END
}