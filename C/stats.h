#pragma once

typedef struct Stats {
	unsigned long mem;
	unsigned long add;
	unsigned long mul;
	unsigned long cmp;
} Stats;

extern Stats statsGlobal;

void statsReset();
Stats statsGet();
void statsPrint(Stats stats);
