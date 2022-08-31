#include "stats.h"
#include <stdio.h>

Stats statsGlobal = {0};

void statsReset() {
	statsGlobal.mem = 0;
	statsGlobal.add = 0;
	statsGlobal.mul = 0;
	statsGlobal.cmp = 0;
}

Stats statsGet() {
	// just copy the global stats so they can be reset
	return (Stats){.mem = statsGlobal.mem,
	               .add = statsGlobal.add,
	               .mul = statsGlobal.mul,
	               .cmp = statsGlobal.cmp};
}

void statsPrint(Stats stats) {
	printf("Stats:\n");
	unsigned long alu_total = stats.add + stats.mul + stats.cmp;
	printf("    RAM transactions: %lu\n", stats.mem);
	printf("    ALU operations:   %lu\n", alu_total);
	printf("        - ADD: %lu\n", stats.add);
	printf("        - MUL: %lu\n", stats.mul);
	printf("        - CMP: %lu\n", stats.cmp);
}
