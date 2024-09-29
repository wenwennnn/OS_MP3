#include "syscall.h"

int test[2000];

int main() {
	int n;
	for (n=15; n<=19; n++) {
		PrintInt(n);
	}
	return 0;
	// Halt();
}

