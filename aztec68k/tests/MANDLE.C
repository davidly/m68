/*
   from http://andreadrian.de/68K-MBC_CPM68K/index.html
   written by Andre Adrian
   CP/M68k 1.3 Alcyon C Compiler + CP/M68k 1.2 LIBF.A or LIBE.A
   MANDLEBRT.BAS
*/

#include <stdio.h>
float atof();

int main()
{
    int X, Y, I;
    float CA, CB, A, B, T;                  
    float cx=atof("0.0458");                
    float cy=atof("0.08333");
    float c2=atof("2");
    float c4=atof("4");
    for (Y= -12; Y<=12; ++Y) {              
        for (X= -39; X<=39; ++X) {          
            CA=X*cx;                        
            CB=Y*cy;                        
            A=CA;                           
            B=CB;                           
            for (I=0; I<=15; ++I) {         
                T=A*A-B*B+CA;               
                B=c2*A*B+CB;                
                A=T;                        
                if ((A*A+B*B)>c4) break;    
            }                               
            if (I>15) {
                printf(" ");                
            } else {                        
                if (I>9) I=I+7;             
                printf("%c", 48+I);         
            }
        }                                   
        printf("\n");                       
    }                                       
}
