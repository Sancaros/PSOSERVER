#pragma once
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Define if you want debugging turned on */
#undef DEBUG

/* Define to 1 if you have the <arpa/inet.h> header file. */
#undef HAVE_ARPA_INET_H

/* Define to 1 if you have the <bsd/libutil.h> header file. */
#undef HAVE_BSD_LIBUTIL_H

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the `getgrouplist' function. */
#undef HAVE_GETGROUPLIST

/* Define to 1 if you have the `gethostname' function. */
#undef HAVE_GETHOSTNAME

/* Define to 1 if you have the `gettimeofday' function. */
#undef HAVE_GETTIMEOFDAY

/* Define to 1 if you have the <grp.h> header file. */
#undef HAVE_GRP_H

/* Define if you have the iconv() function and it works. */
#undef HAVE_ICONV

/* Define to 1 if you have the `inet_ntoa' function. */
#undef HAVE_INET_NTOA

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the `mini18n' library (-lmini18n). */
#undef HAVE_LIBMINI18N

/* Define to 1 if you have the `psoarchive' library (-lpsoarchive). */
#undef HAVE_LIBPSOARCHIVE

/* Define to 1 if you have the `sylverant' library (-lsylverant). */
#undef HAVE_LIBSYLVERANT

/* Define to 1 if you have the <libutil.h> header file. */
#undef HAVE_LIBUTIL_H

/* Define to 1 if you have the `malloc' function. */
#undef HAVE_MALLOC

/* Define to 1 if you have the <math.h> header file. */
#undef HAVE_MATH_H

/* Define to 1 if you have the `memmove' function. */
#undef HAVE_MEMMOVE

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define to 1 if you have the `memset' function. */
#undef HAVE_MEMSET

/* Define to 1 if you have the <netdb.h> header file. */
#undef HAVE_NETDB_H

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#undef HAVE_NETINET_TCP_H

/* Define if you have POSIX threads libraries and header files. */
#undef HAVE_PTHREAD

/* Define to 1 if you have the <pwd.h> header file. */
#undef HAVE_PWD_H

/* Define to 1 if you have the `realloc' function. */
#undef HAVE_REALLOC

/* Define to 1 if you have the `select' function. */
#undef HAVE_SELECT

/* Define to 1 if you have the `socket' function. */
#undef HAVE_SOCKET

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#undef HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#undef HAVE_STRING_H

/* Define to 1 if you have the `strtoul' function. */
#undef HAVE_STRTOUL

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Define to 1 if you have the <sys/utsname.h> header file. */
#undef HAVE_SYS_UTSNAME_H

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define as const if the declaration of iconv() needs const. */
#undef ICONV_CONST

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#undef LT_OBJDIR

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
#undef PTHREAD_CREATE_JOINABLE

   /* The size of `double', as computed by sizeof. */
#undef SIZEOF_DOUBLE

/* The size of `float', as computed by sizeof. */
#undef SIZEOF_FLOAT

/* The size of `int', as computed by sizeof. */
#undef SIZEOF_INT

/* The size of `long int', as computed by sizeof. */
#undef SIZEOF_LONG_INT

/* The size of `void *', as computed by sizeof. */
#undef SIZEOF_VOID_P

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Define if you want IPv6 support */
#undef PSOCN_ENABLE_IPV6

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

   /* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
	  <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
	  #define below would cause a syntax error. */
#undef _UINT32_T

	  /* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
		 <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
		 #define below would cause a syntax error. */
#undef _UINT8_T

		 /* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif

   /* Define to the equivalent of the C99 'restrict' keyword, or to
	  nothing if this is not supported.  Do not define if restrict is
	  supported directly.  */
#undef restrict
	  /* Work around a bug in Sun C++: it does not support _Restrict or
		 __restrict__, even though the corresponding Sun C compiler ends up with
		 "#define restrict _Restrict" or "#define restrict __restrict__" in the
		 previous line.  Perhaps some future version of Sun C++ will work with
		 restrict; if so, hopefully it defines __RESTRICT like Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
# define __restrict__
#endif

		 /* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t

/* Define to `int' if <sys/types.h> does not define. */
#undef ssize_t

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
#undef uint16_t

   /* Define to the type of an unsigned integer type of width exactly 32 bits if
	  such a type exists and the standard includes do not define it. */
#undef uint32_t

	  /* Define to the type of an unsigned integer type of width exactly 8 bits if
		 such a type exists and the standard includes do not define it. */
#undef uint8_t