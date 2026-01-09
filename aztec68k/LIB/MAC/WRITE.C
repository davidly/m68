struct IOParam {
	char *qLink; 
	short qType; 
	short ioTrap; 
	char *ioCmdAddr; 
	char *ioCompletion;
	short ioResult; 
	char *ioNamePtr;
	short ioVRefNum;
	short ioRefNum;
	char ioVersNum;
	char ioPermssn;
	char * ioMisc;
	char *ioBuffer;
	long ioReqCount;
	long ioActCount;
	short ioPosMode;
	long ioPosOffset;
};

int errno;
int connum;

write(fd, buf, len)
char *buf;
{
	register int *refnums, err;
	struct IOParam pb;

	pb.ioRefNum = connum;
	pb.ioBuffer = buf;
	pb.ioReqCount = len;
	pb.ioPosMode = 0;
	if (err = PBWrite(&pb, 0)){
		errno = err;
		return(-1);
	}
	return((int)pb.ioActCount);	
}

