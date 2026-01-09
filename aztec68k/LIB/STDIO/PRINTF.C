/* Copyright (C) 1981,1982 by Manx Software Systems */

printf(fmt,args)
char *fmt; unsigned args;
{
	int putchar();

	return(format(putchar,fmt,&args));
}

