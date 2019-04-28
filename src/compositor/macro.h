#ifndef _MACRO_H_
#define _MACRO_H_

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
#else

# define debug(fmt, arg...)
# define debugv(loglvl, fmt, arg...)
#endif

#endif
