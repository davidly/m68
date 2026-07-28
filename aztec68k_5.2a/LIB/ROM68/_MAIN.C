#include <stdlib.h>

int main(int argc, char **argv);

void
_main(void)
{
	static char *Argv[] = {0};

	exit(main(0, Argv));
}
