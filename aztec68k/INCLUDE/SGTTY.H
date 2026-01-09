/* Copyright (C) 1984 by Manx Software Systems, Inc. */

#define TIOCGETP	0		/* read contents of tty control structure */
#define TIOCSETP	1		/* set contents of tty control structure */
#define TIOCSETN	2		/* ditto only don't wait for output to flush */
#define	TIOCNTLC	3		/* check for control C */

struct sgttyb {
	short sg_flags;		/* control flags */
	char sg_erase;
	char sg_kill;
};

/* settings for flags */
#define	TABSIZ	0x000f	/* size of tab if XTABS is set */
#define	XTABS	0x0010	/* convert tabs to spaces on output */
#define	CRMOD	0x0020	/* CR -> LF on input, LF -> CR/LF on output */
#define	ECHO	0x0040	/* echo input to output */
#define	ECHOE	0x0080	/* follow erase character by space, BS */
#define	CBREAK	0x0100	/* don't wait for the whole line */
#define	NODELAY	0x0200	/* return even if no characters */
#define RAW		0x0400	/* no input or output mapping at all */
#define	CHKKEY	0x0800	/* check for keyboard characters during output */
#define	ERASE	0x1000	/* erase character space before drawing */

