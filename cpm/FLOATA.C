// from http://andreadrian.de/68K-MBC_CPM68K/index.html
// written by Andre Adrian
// CP/M68k 1.3 Alcyon C Compiler + CP/M68k 1.2 LIBF.A or LIBE.A
#include <stdio.h>

extern float atof();

static float cf[] = {    // fail
    -9.9999999E-01 ,  -1.00000011E00 ,  -1.0000002E00 ,  0 ,
     9.9999999E-01 ,   1.00000011E00 ,   1.0000002E00 ,
    -1e-19, -7e18, 1e-19, 7e18,
};

static long cl[] = {     // okay
    0xFFFFFFC0, 0x800000C1, 0x800001C1, 0,  // libf
    0xFFFFFF40, 0x80000041, 0x80000141,
    0x80000081, 0xFFFFFFFF, 0x80000001, 0xFFFFFF7F,
};

static char cs[][16] = { // okay
    "-9.9999999E-01", "-1.00000011E00", "-1.0000002E00", "0",
     "9.9999999E-01",  "1.00000011E00",  "1.0000002E00",
    "-1e-19", "-7e18", "1e-19", "7e18",
};

// floating point simple "pocket calculator"
void calculator() {
    char c;
    float f1, f2, f3;

    printf("\nEnter float op float, e.g. 0.6 + 0.7\n");
    for (;;) {
        scanf("%f %c %f", &f1, &c, &f2);
        switch(c) {
            case '+': f3 = f1 + f2; break;
            case '-': f3 = f1 - f2; break;
            case '*': f3 = f1 * f2; break;
            case '/': f3 = f1 / f2; break;
            default: printf("wrong op: %c\n", c); return; break;
        }
        printf("%14.7e %c %14.7e = %14.7e\n", f1, c, f2, f3);
        printf("0x%08lx %c 0x%08lx = 0x%08lx\n", f1, c, f2, f3);
    }
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    long l;
    float f;
    double d;

    printf("sizeof int %d\n", sizeof(i));
    printf("sizeof long %d\n", sizeof(l));
    printf("sizeof float %d\n", sizeof(f));
    printf("sizeof double %d\n", sizeof(d));

    printf("\nfloat constants: fail\n");
    for (i = 0; i < sizeof(cf)/sizeof(cf[0]); ++i) {
        printf("0x%08lx %14.7e\n", cf[i], cf[i]);
    }
    printf("\nstring constants used as float constants: okay\n");
    for (i = 0; i < sizeof(cs)/sizeof(cs[0]); ++i) {
        f = atof(cs[i]);
        printf("%15s 0x%08lx %14.7e\n", cs[i], f, f);
    }
    printf("\nlong constants used as float constants: okay\n");
    for (i = 0; i < sizeof(cl)/sizeof(cl[0]); ++i) {
        printf("0x%08lx %14.7e\n", cl[i], cl[i]);
    }
#if 0
    calculator();
#endif
}
