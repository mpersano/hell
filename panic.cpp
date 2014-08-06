#include <cstdio>
#include <cstdlib>

#include <stdarg.h>

#ifdef ANDROID_NDK
#include <android/log.h>
#endif

#include "panic.h"

void
panic(const char *fmt, ...)
{
	char buf[512];

	va_list ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

#ifndef ANDROID_NDK
	fprintf(stderr, "FATAL: %s\n", buf);
#else
	__android_log_print(ANDROID_LOG_INFO, "kpuzzle", "FATAL: %s", buf);
#endif

	exit(1);
}
