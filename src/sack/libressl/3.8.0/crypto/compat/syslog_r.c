#include <syslog.h>

#ifdef vsyslog_r 
#undef vsyslog_r
#undef syslog_r
#endif

void
syslog_r(int pri, struct syslog_data *data, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsyslog_r(pri, data, fmt, ap);
    va_end(ap);
}

void
vsyslog_r(int pri, struct syslog_data *data, const char *fmt, va_list ap)
{
#ifdef HAVE_SYSLOG
	vsyslog(pri, fmt, ap);
#endif
}
