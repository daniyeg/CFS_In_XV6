#include "types.h"
#include "user.h"

// User program to exit out of the OS using halt systemcall.
int main(int argc, char *argv[])
{
	halt();
	exit();
}
