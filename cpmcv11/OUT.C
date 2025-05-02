out(port,c)
unsigned port,c;
	{
	char *p;
	p = (char *)((port & 0xff) + 0xff0000);
	p[0] = c;
}
