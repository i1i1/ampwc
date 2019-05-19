#ifndef _MACRO_H_
#define _MACRO_H_

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define ARRSZ(arr) (sizeof(arr) / sizeof(arr[0]))

#define CRLF	"\r\n"

#define LOG_FULL(log_fmt) "%s:%d %s" log_fmt CRLF,		\
			__FILE__, 				\
			__LINE__, 				\
			__FUNCTION__
#define LOG_SHORT(log_fmt) "%20s| " log_fmt CRLF,  __FUNCTION__

#define LOG_FMT LOG_SHORT

#define _int_logit(file, prefix, log_fmt, arg...) 		\
do { 								\
	fprintf(file, prefix LOG_FMT(log_fmt),			\
			##arg);					\
} while (0);

#define error(status, fmt, arg...) 				\
do { 								\
	_int_logit(stderr, "error: ", fmt, ##arg);		\
	fflush(stderr); 					\
	exit(status);						\
} while (0)

#define warning(fmt, arg...) 					\
do { 								\
	_int_logit(stderr, "[-] ", fmt, ##arg);			\
	fflush(stderr); 					\
} while (0)

#define log_default 1
#define log_verbose 2

#ifndef NDEBUG

#  ifndef LOG_LEVEL
#    define LOG_LEVEL log_verbose
#  endif

#  define debug(fmt, arg...) 					\
   do {								\
	_int_logit(stdout, "[+] ", fmt, ##arg);			\
   } while (0)

#  define debugv(loglvl, fmt, arg...)				\
   do {								\
	   if (loglvl <= LOG_LEVEL) {				\
		_int_logit(stdout, "[+] ", fmt, ##arg);		\
	   }							\
   } while (0)
#else //NDEBUG

# define debug(fmt, arg...)
# define debugv(loglvl, fmt, arg...)
#endif //NDEBUG

static inline void*
xmalloc(size_t size)
{
	void *ret;

	errno = 0;
	ret = malloc(size);
	if (size != 0 && ret == NULL) {
		perror("malloc()");

		exit(EXIT_FAILURE);
	}

	return ret;
}

static inline void*
xrealloc(void *ptr, size_t size)
{
	void *ret;

	ret = realloc(ptr, size);
	if (size != 0 && ret == NULL) {
		perror("realloc()");

		exit(EXIT_FAILURE);
	}

	return ret;
}

#endif //_MACRO_H_

