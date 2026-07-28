#include <signal.h>
#include <errno.h>
#include <fcntl.h>

void (*_sigfuns[_NUMSIG])(int);
static char setup;
static void _sig_setup(void);

void (*signal(register int sig, void (*func)(int)))(int)
{
	register void (*retval)(int);

	if (!setup) {
		_sig_setup();
		setup = 1;
	}
	if ((sig -= _FSTSIG) < 0 || sig >= _NUMSIG) {
		errno = EINVAL;
		return SIG_ERR;
	}
	retval = _sigfuns[sig];
	_sigfuns[sig] = func;
	return retval;
}

int
raise(int sig)
{
	register void (*handler)(int);
	register int tsig = sig - _FSTSIG;

	if (tsig < 0 || tsig >= _NUMSIG)
		return(-1);
	if ((handler = _sigfuns[tsig]) == SIG_DFL)
		_exit(255);
	if (handler != SIG_IGN) {
		_sigfuns[tsig] = SIG_DFL;
		(*handler)(sig);
	}
	return(0);
}

static void
_sig_setup(void)
{
}
