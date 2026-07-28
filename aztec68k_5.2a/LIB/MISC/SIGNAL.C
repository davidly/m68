#if MCH_AMIGA
#include "amisig.c"
#elif MCH_MACINTOSH
#include "macsig.c"
#else
#include "romsig.c"
#endif
