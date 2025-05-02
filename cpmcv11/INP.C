unsigned inp(port)
unsigned port;
	{
	char *p;
	p = (char *)((port & 0xff) + 0xff0000);
	return(p[0]);
}
