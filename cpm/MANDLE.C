// from http://andreadrian.de/68K-MBC_CPM68K/index.html
// written by Andre Adrian
// CP/M68k 1.3 Alcyon C Compiler + CP/M68k 1.2 LIBF.A or LIBE.A
// MANDLEBRT.BAS
#include <stdio.h>
#ifdef CPM
float atof();
#else
#include <stdlib.h>
#endif // CPM
int main()
{
    int X, Y, I;
    float CA, CB, A, B, T;                  //MANDLEBRT.BAS
    float cx=atof("0.0458");                //workaround Alycon C bugs
    float cy=atof("0.08333");
    float c2=atof("2");
    float c4=atof("4");
    for (Y= -12; Y<=12; ++Y) {              //10 FOR Y=-12 TO 12
        for (X= -39; X<=39; ++X) {          //20 FOR X=-39 TO 39
            CA=X*cx;                        //30 CA=X*0.0458
            CB=Y*cy;                        //40 CB=Y*0.08333
            A=CA;                           //50 A=CA
            B=CB;                           //60 B=CB
            for (I=0; I<=15; ++I) {         //70 FOR I=0 TO 15
                T=A*A-B*B+CA;               //80 T=A*A-B*B+CA
                B=c2*A*B+CB;                //90 B=2*A*B+CB
                A=T;                        //100 A=T
                if ((A*A+B*B)>c4) break;    //110 IF (A*A+B*B)>4 GOTO200
            }                               //120 NEXT I
            if (I>15) {
                printf(" ");                //130 PRINT " ";
            } else {                        //140 GOTO 210
                if (I>9) I=I+7;             //200 IF I>9 THEN I=I+7
                printf("%c", 48+I);         //205 PRINT CHR$(48+I);
            }
        }                                   //210 NEXT X
        printf("\n");                       //220 PRINT
    }                                       //230 NEXT Y
}
