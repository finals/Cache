#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __linux__
#define HAVE_EPOLL 1
#endif

#if (__i386 || __amd64) && __GNUC__
#define GNUC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GNUC_VERSION >= 40100
#define GCC_ATOMIC
#endif
#endif

/* 是否开启多线程 */
#ifndef ENABLE_THREAD
#define ENABLE_THREAD  0
#endif


#ifndef NULL
#define NULL ((void*)0)
#endif

#endif
