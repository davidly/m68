write (fd, buf, cnt)
int fd;
register int cnt;
register char *buf;
{
	register int i;

	for(i=0;i<cnt;i++)
		bdos(2,buf[i]);
	return cnt;
}
