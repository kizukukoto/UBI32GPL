# 1 "nsswitch/wb_common.c"
# 1 "/home/cem/ubicom/uClinux/user/samba/source//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "nsswitch/wb_common.c"
# 27 "nsswitch/wb_common.c"
# 1 "nsswitch/winbind_client.h" 1
# 1 "nsswitch/winbind_nss_config.h" 1
# 33 "nsswitch/winbind_nss_config.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 1
# 28 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/features.h" 1
# 35 "/home/cem/ubicom/uClinux/uClibc/include/features.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_config.h" 1
# 36 "/home/cem/ubicom/uClinux/uClibc/include/features.h" 2

# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_arch_features.h" 1
# 38 "/home/cem/ubicom/uClinux/uClibc/include/features.h" 2
# 356 "/home/cem/ubicom/uClinux/uClibc/include/features.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/sys/cdefs.h" 1
# 357 "/home/cem/ubicom/uClinux/uClibc/include/features.h" 2
# 29 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2





# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 1 3 4
# 214 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 35 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2

# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 1
# 28 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/wordsize.h" 1
# 29 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 2


# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 1 3 4
# 32 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/kernel_types.h" 1
# 10 "/home/cem/ubicom/uClinux/uClibc/include/bits/kernel_types.h"
typedef unsigned long __kernel_dev_t;
typedef unsigned long __kernel_ino_t;
typedef unsigned short __kernel_mode_t;
typedef unsigned short __kernel_nlink_t;
typedef long __kernel_off_t;
typedef int __kernel_pid_t;
typedef unsigned short __kernel_ipc_pid_t;
typedef unsigned short __kernel_uid_t;
typedef unsigned short __kernel_gid_t;
typedef unsigned int __kernel_size_t;
typedef int __kernel_ssize_t;
typedef int __kernel_ptrdiff_t;
typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;
typedef long __kernel_clock_t;
typedef int __kernel_daddr_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;
typedef unsigned short __kernel_old_dev_t;
typedef long long __kernel_loff_t;

typedef struct {



 int __val[2];

} __kernel_fsid_t;
# 33 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 2


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;




__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;







__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
# 135 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/typesizes.h" 1
# 136 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 2


__extension__ typedef __u_quad_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __u_quad_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __quad_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __u_quad_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;

__extension__ typedef int __daddr_t;
__extension__ typedef long int __swblk_t;
__extension__ typedef int __key_t;


__extension__ typedef int __clockid_t;


__extension__ typedef void * __timer_t;


__extension__ typedef long int __blksize_t;




__extension__ typedef long int __blkcnt_t;
__extension__ typedef __quad_t __blkcnt64_t;


__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __u_quad_t __fsblkcnt64_t;


__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __u_quad_t __fsfilcnt64_t;

__extension__ typedef int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


__extension__ typedef int __intptr_t;


__extension__ typedef unsigned int __socklen_t;





typedef __kernel_ipc_pid_t __ipc_pid_t;



# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/pthreadtypes.h" 1
# 23 "/home/cem/ubicom/uClinux/uClibc/include/bits/pthreadtypes.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/sched.h" 1
# 91 "/home/cem/ubicom/uClinux/uClibc/include/bits/sched.h"
struct __sched_param
  {
    int __sched_priority;
  };
# 24 "/home/cem/ubicom/uClinux/uClibc/include/bits/pthreadtypes.h" 2


struct _pthread_fastlock
{
  long int __status;
  int __spinlock;

};



typedef struct _pthread_descr_struct *_pthread_descr;





typedef struct __pthread_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pthread_attr_t;



typedef struct
{
  struct _pthread_fastlock __c_lock;
  _pthread_descr __c_waiting;
} pthread_cond_t;



typedef struct
{
  int __dummy;
} pthread_condattr_t;


typedef unsigned int pthread_key_t;





typedef struct
{
  int __m_reserved;
  int __m_count;
  _pthread_descr __m_owner;
  int __m_kind;
  struct _pthread_fastlock __m_lock;
} pthread_mutex_t;



typedef struct
{
  int __mutexkind;
} pthread_mutexattr_t;



typedef int pthread_once_t;




typedef struct _pthread_rwlock_t
{
  struct _pthread_fastlock __rw_lock;
  int __rw_readers;
  _pthread_descr __rw_writer;
  _pthread_descr __rw_read_waiting;
  _pthread_descr __rw_write_waiting;
  int __rw_kind;
  int __rw_pshared;
} pthread_rwlock_t;



typedef struct
{
  int __lockkind;
  int __pshared;
} pthread_rwlockattr_t;




typedef volatile int pthread_spinlock_t;


typedef struct {
  struct _pthread_fastlock __ba_lock;
  int __ba_required;
  int __ba_present;
  _pthread_descr __ba_waiting;
} pthread_barrier_t;


typedef struct {
  int __pshared;
} pthread_barrierattr_t;





typedef unsigned long int pthread_t;
# 207 "/home/cem/ubicom/uClinux/uClibc/include/bits/types.h" 2
# 37 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2









typedef struct __STDIO_FILE_STRUCT FILE;





# 62 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
typedef struct __STDIO_FILE_STRUCT __FILE;
# 72 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h" 1
# 119 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_mutex.h" 1
# 15 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_mutex.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 1
# 20 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/sched.h" 1
# 29 "/home/cem/ubicom/uClinux/uClibc/include/sched.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/time.h" 1
# 121 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
# 30 "/home/cem/ubicom/uClinux/uClibc/include/sched.h" 2


# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/sched.h" 1
# 65 "/home/cem/ubicom/uClinux/uClibc/include/bits/sched.h"
struct sched_param
  {
    int __sched_priority;
  };





extern int clone (int (*__fn) (void *__arg), void *__child_stack,
    int __flags, void *__arg, ...) __attribute__ ((__nothrow__));








# 106 "/home/cem/ubicom/uClinux/uClibc/include/bits/sched.h"
typedef unsigned long int __cpu_mask;






typedef struct
{
  __cpu_mask __bits[1024 / (8 * sizeof (__cpu_mask))];
} cpu_set_t;
# 33 "/home/cem/ubicom/uClinux/uClibc/include/sched.h" 2







extern int sched_setparam (__pid_t __pid, __const struct sched_param *__param)
     __attribute__ ((__nothrow__));


extern int sched_getparam (__pid_t __pid, struct sched_param *__param) __attribute__ ((__nothrow__));


extern int sched_setscheduler (__pid_t __pid, int __policy,
          __const struct sched_param *__param) __attribute__ ((__nothrow__));


extern int sched_getscheduler (__pid_t __pid) __attribute__ ((__nothrow__));


extern int sched_yield (void) __attribute__ ((__nothrow__));


extern int sched_get_priority_max (int __algorithm) __attribute__ ((__nothrow__));


extern int sched_get_priority_min (int __algorithm) __attribute__ ((__nothrow__));


extern int sched_rr_get_interval (__pid_t __pid, struct timespec *__t) __attribute__ ((__nothrow__));
# 76 "/home/cem/ubicom/uClinux/uClibc/include/sched.h"
extern int sched_setaffinity (__pid_t __pid, size_t __cpusetsize,
         __const cpu_set_t *__cpuset) __attribute__ ((__nothrow__));


extern int sched_getaffinity (__pid_t __pid, size_t __cpusetsize,
         cpu_set_t *__cpuset) __attribute__ ((__nothrow__));



# 21 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/time.h" 1
# 31 "/home/cem/ubicom/uClinux/uClibc/include/time.h"








# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 1 3 4
# 40 "/home/cem/ubicom/uClinux/uClibc/include/time.h" 2



# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/time.h" 1
# 38 "/home/cem/ubicom/uClinux/uClibc/include/bits/time.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_clk_tck.h" 1
# 39 "/home/cem/ubicom/uClinux/uClibc/include/bits/time.h" 2
# 44 "/home/cem/ubicom/uClinux/uClibc/include/time.h" 2
# 59 "/home/cem/ubicom/uClinux/uClibc/include/time.h"


typedef __clock_t clock_t;



# 75 "/home/cem/ubicom/uClinux/uClibc/include/time.h"


typedef __time_t time_t;



# 93 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
typedef __clockid_t clockid_t;
# 105 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
typedef __timer_t timer_t;
# 132 "/home/cem/ubicom/uClinux/uClibc/include/time.h"


struct tm
{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;



  long int tm_gmtoff;
  __const char *tm_zone;





};








struct itimerspec
  {
    struct timespec it_interval;
    struct timespec it_value;
  };


struct sigevent;





typedef __pid_t pid_t;








extern clock_t clock (void) __attribute__ ((__nothrow__));


extern time_t time (time_t *__timer) __attribute__ ((__nothrow__));



extern double difftime (time_t __time1, time_t __time0)
     __attribute__ ((__nothrow__)) __attribute__ ((__const__));



extern time_t mktime (struct tm *__tp) __attribute__ ((__nothrow__));





extern size_t strftime (char *__restrict __s, size_t __maxsize,
   __const char *__restrict __format,
   __const struct tm *__restrict __tp) __attribute__ ((__nothrow__));





extern char *strptime (__const char *__restrict __s,
         __const char *__restrict __fmt, struct tm *__tp)
     __attribute__ ((__nothrow__));
# 235 "/home/cem/ubicom/uClinux/uClibc/include/time.h"



extern struct tm *gmtime (__const time_t *__timer) __attribute__ ((__nothrow__));



extern struct tm *localtime (__const time_t *__timer) __attribute__ ((__nothrow__));





extern struct tm *gmtime_r (__const time_t *__restrict __timer,
       struct tm *__restrict __tp) __attribute__ ((__nothrow__));



extern struct tm *localtime_r (__const time_t *__restrict __timer,
          struct tm *__restrict __tp) __attribute__ ((__nothrow__));





extern char *asctime (__const struct tm *__tp) __attribute__ ((__nothrow__));


extern char *ctime (__const time_t *__timer) __attribute__ ((__nothrow__));







extern char *asctime_r (__const struct tm *__restrict __tp,
   char *__restrict __buf) __attribute__ ((__nothrow__));


extern char *ctime_r (__const time_t *__restrict __timer,
        char *__restrict __buf) __attribute__ ((__nothrow__));
# 291 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
extern char *tzname[2];



extern void tzset (void) __attribute__ ((__nothrow__));



extern int daylight;
extern long int timezone;





extern int stime (__const time_t *__when) __attribute__ ((__nothrow__));
# 321 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
extern time_t timegm (struct tm *__tp) __attribute__ ((__nothrow__));


extern time_t timelocal (struct tm *__tp) __attribute__ ((__nothrow__));


extern int dysize (int __year) __attribute__ ((__nothrow__)) __attribute__ ((__const__));
# 336 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
extern int nanosleep (__const struct timespec *__requested_time,
        struct timespec *__remaining);



extern int clock_getres (clockid_t __clock_id, struct timespec *__res) __attribute__ ((__nothrow__));


extern int clock_gettime (clockid_t __clock_id, struct timespec *__tp) __attribute__ ((__nothrow__));


extern int clock_settime (clockid_t __clock_id, __const struct timespec *__tp)
     __attribute__ ((__nothrow__));
# 368 "/home/cem/ubicom/uClinux/uClibc/include/time.h"
extern int timer_create (clockid_t __clock_id,
    struct sigevent *__restrict __evp,
    timer_t *__restrict __timerid) __attribute__ ((__nothrow__));


extern int timer_delete (timer_t __timerid) __attribute__ ((__nothrow__));


extern int timer_settime (timer_t __timerid, int __flags,
     __const struct itimerspec *__restrict __value,
     struct itimerspec *__restrict __ovalue) __attribute__ ((__nothrow__));


extern int timer_gettime (timer_t __timerid, struct itimerspec *__value)
     __attribute__ ((__nothrow__));


extern int timer_getoverrun (timer_t __timerid) __attribute__ ((__nothrow__));
# 431 "/home/cem/ubicom/uClinux/uClibc/include/time.h"

# 22 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2


# 1 "/home/cem/ubicom/uClinux/uClibc/include/signal.h" 1
# 31 "/home/cem/ubicom/uClinux/uClibc/include/signal.h"


# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/sigset.h" 1
# 23 "/home/cem/ubicom/uClinux/uClibc/include/bits/sigset.h"
typedef int __sig_atomic_t;




typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
# 34 "/home/cem/ubicom/uClinux/uClibc/include/signal.h" 2
# 50 "/home/cem/ubicom/uClinux/uClibc/include/signal.h"
typedef __sigset_t sigset_t;
# 399 "/home/cem/ubicom/uClinux/uClibc/include/signal.h"

# 25 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/pthreadtypes.h" 1
# 26 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/initspin.h" 1
# 27 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2



# 59 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
enum
{
  PTHREAD_CREATE_JOINABLE,

  PTHREAD_CREATE_DETACHED

};

enum
{
  PTHREAD_INHERIT_SCHED,

  PTHREAD_EXPLICIT_SCHED

};

enum
{
  PTHREAD_SCOPE_SYSTEM,

  PTHREAD_SCOPE_PROCESS

};

enum
{
  PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_TIMED_NP

  ,
  PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL



  , PTHREAD_MUTEX_FAST_NP = PTHREAD_MUTEX_ADAPTIVE_NP

};

enum
{
  PTHREAD_PROCESS_PRIVATE,

  PTHREAD_PROCESS_SHARED

};


enum
{
  PTHREAD_RWLOCK_PREFER_READER_NP,
  PTHREAD_RWLOCK_PREFER_WRITER_NP,
  PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
  PTHREAD_RWLOCK_DEFAULT_NP = PTHREAD_RWLOCK_PREFER_WRITER_NP
};
# 131 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
struct _pthread_cleanup_buffer
{
  void (*__routine) (void *);
  void *__arg;
  int __canceltype;
  struct _pthread_cleanup_buffer *__prev;
};



enum
{
  PTHREAD_CANCEL_ENABLE,

  PTHREAD_CANCEL_DISABLE

};
enum
{
  PTHREAD_CANCEL_DEFERRED,

  PTHREAD_CANCEL_ASYNCHRONOUS

};
# 163 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern int pthread_create (pthread_t *__restrict __threadp,
      __const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__));


extern pthread_t pthread_self (void) __attribute__ ((__nothrow__));


extern int pthread_equal (pthread_t __thread1, pthread_t __thread2) __attribute__ ((__nothrow__));


extern void pthread_exit (void *__retval) __attribute__ ((__noreturn__));




extern int pthread_join (pthread_t __th, void **__thread_return);





extern int pthread_detach (pthread_t __th) __attribute__ ((__nothrow__));







extern int pthread_attr_init (pthread_attr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_attr_destroy (pthread_attr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_attr_setdetachstate (pthread_attr_t *__attr,
     int __detachstate) __attribute__ ((__nothrow__));


extern int pthread_attr_getdetachstate (__const pthread_attr_t *__attr,
     int *__detachstate) __attribute__ ((__nothrow__));


extern int pthread_attr_setschedparam (pthread_attr_t *__restrict __attr,
           __const struct sched_param *__restrict
           __param) __attribute__ ((__nothrow__));


extern int pthread_attr_getschedparam (__const pthread_attr_t *__restrict
           __attr,
           struct sched_param *__restrict __param)
     __attribute__ ((__nothrow__));


extern int pthread_attr_setschedpolicy (pthread_attr_t *__attr, int __policy)
     __attribute__ ((__nothrow__));


extern int pthread_attr_getschedpolicy (__const pthread_attr_t *__restrict
     __attr, int *__restrict __policy)
     __attribute__ ((__nothrow__));


extern int pthread_attr_setinheritsched (pthread_attr_t *__attr,
      int __inherit) __attribute__ ((__nothrow__));


extern int pthread_attr_getinheritsched (__const pthread_attr_t *__restrict
      __attr, int *__restrict __inherit)
     __attribute__ ((__nothrow__));


extern int pthread_attr_setscope (pthread_attr_t *__attr, int __scope)
     __attribute__ ((__nothrow__));


extern int pthread_attr_getscope (__const pthread_attr_t *__restrict __attr,
      int *__restrict __scope) __attribute__ ((__nothrow__));



extern int pthread_attr_setguardsize (pthread_attr_t *__attr,
          size_t __guardsize) __attribute__ ((__nothrow__));


extern int pthread_attr_getguardsize (__const pthread_attr_t *__restrict
          __attr, size_t *__restrict __guardsize)
     __attribute__ ((__nothrow__));






extern int pthread_attr_setstackaddr (pthread_attr_t *__attr,
          void *__stackaddr) __attribute__ ((__nothrow__));


extern int pthread_attr_getstackaddr (__const pthread_attr_t *__restrict
          __attr, void **__restrict __stackaddr)
     __attribute__ ((__nothrow__));





extern int pthread_attr_setstack (pthread_attr_t *__attr, void *__stackaddr,
      size_t __stacksize) __attribute__ ((__nothrow__));


extern int pthread_attr_getstack (__const pthread_attr_t *__restrict __attr,
      void **__restrict __stackaddr,
      size_t *__restrict __stacksize) __attribute__ ((__nothrow__));





extern int pthread_attr_setstacksize (pthread_attr_t *__attr,
          size_t __stacksize) __attribute__ ((__nothrow__));


extern int pthread_attr_getstacksize (__const pthread_attr_t *__restrict
          __attr, size_t *__restrict __stacksize)
     __attribute__ ((__nothrow__));
# 306 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern int pthread_setschedparam (pthread_t __target_thread, int __policy,
      __const struct sched_param *__param)
     __attribute__ ((__nothrow__));


extern int pthread_getschedparam (pthread_t __target_thread,
      int *__restrict __policy,
      struct sched_param *__restrict __param)
     __attribute__ ((__nothrow__));



extern int pthread_getconcurrency (void) __attribute__ ((__nothrow__));


extern int pthread_setconcurrency (int __level) __attribute__ ((__nothrow__));






extern int pthread_mutex_init (pthread_mutex_t *__restrict __mutex,
          __const pthread_mutexattr_t *__restrict
          __mutex_attr) __attribute__ ((__nothrow__));


extern int pthread_mutex_destroy (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__));


extern int pthread_mutex_trylock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__));


extern int pthread_mutex_lock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__));



extern int pthread_mutex_timedlock (pthread_mutex_t *__restrict __mutex,
        __const struct timespec *__restrict
        __abstime) __attribute__ ((__nothrow__));



extern int pthread_mutex_unlock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__));






extern int pthread_mutexattr_init (pthread_mutexattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_mutexattr_destroy (pthread_mutexattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_mutexattr_getpshared (__const pthread_mutexattr_t *
      __restrict __attr,
      int *__restrict __pshared) __attribute__ ((__nothrow__));


extern int pthread_mutexattr_setpshared (pthread_mutexattr_t *__attr,
      int __pshared) __attribute__ ((__nothrow__));





extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind)
     __attribute__ ((__nothrow__));


extern int pthread_mutexattr_gettype (__const pthread_mutexattr_t *__restrict
          __attr, int *__restrict __kind) __attribute__ ((__nothrow__));







extern int pthread_cond_init (pthread_cond_t *__restrict __cond,
         __const pthread_condattr_t *__restrict
         __cond_attr) __attribute__ ((__nothrow__));


extern int pthread_cond_destroy (pthread_cond_t *__cond) __attribute__ ((__nothrow__));


extern int pthread_cond_signal (pthread_cond_t *__cond) __attribute__ ((__nothrow__));


extern int pthread_cond_broadcast (pthread_cond_t *__cond) __attribute__ ((__nothrow__));



extern int pthread_cond_wait (pthread_cond_t *__restrict __cond,
         pthread_mutex_t *__restrict __mutex);





extern int pthread_cond_timedwait (pthread_cond_t *__restrict __cond,
       pthread_mutex_t *__restrict __mutex,
       __const struct timespec *__restrict
       __abstime);




extern int pthread_condattr_init (pthread_condattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_condattr_destroy (pthread_condattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_condattr_getpshared (__const pthread_condattr_t *
     __restrict __attr,
     int *__restrict __pshared) __attribute__ ((__nothrow__));


extern int pthread_condattr_setpshared (pthread_condattr_t *__attr,
     int __pshared) __attribute__ ((__nothrow__));







extern int pthread_rwlock_init (pthread_rwlock_t *__restrict __rwlock,
    __const pthread_rwlockattr_t *__restrict
    __attr) __attribute__ ((__nothrow__));


extern int pthread_rwlock_destroy (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));


extern int pthread_rwlock_rdlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));


extern int pthread_rwlock_tryrdlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));



extern int pthread_rwlock_timedrdlock (pthread_rwlock_t *__restrict __rwlock,
           __const struct timespec *__restrict
           __abstime) __attribute__ ((__nothrow__));



extern int pthread_rwlock_wrlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));


extern int pthread_rwlock_trywrlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));



extern int pthread_rwlock_timedwrlock (pthread_rwlock_t *__restrict __rwlock,
           __const struct timespec *__restrict
           __abstime) __attribute__ ((__nothrow__));



extern int pthread_rwlock_unlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__));





extern int pthread_rwlockattr_init (pthread_rwlockattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_rwlockattr_destroy (pthread_rwlockattr_t *__attr) __attribute__ ((__nothrow__));


extern int pthread_rwlockattr_getpshared (__const pthread_rwlockattr_t *
       __restrict __attr,
       int *__restrict __pshared) __attribute__ ((__nothrow__));


extern int pthread_rwlockattr_setpshared (pthread_rwlockattr_t *__attr,
       int __pshared) __attribute__ ((__nothrow__));


extern int pthread_rwlockattr_getkind_np (__const pthread_rwlockattr_t *__attr,
       int *__pref) __attribute__ ((__nothrow__));


extern int pthread_rwlockattr_setkind_np (pthread_rwlockattr_t *__attr,
       int __pref) __attribute__ ((__nothrow__));
# 557 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern int pthread_key_create (pthread_key_t *__key,
          void (*__destr_function) (void *)) __attribute__ ((__nothrow__));


extern int pthread_key_delete (pthread_key_t __key) __attribute__ ((__nothrow__));


extern int pthread_setspecific (pthread_key_t __key,
    __const void *__pointer) __attribute__ ((__nothrow__));


extern void *pthread_getspecific (pthread_key_t __key) __attribute__ ((__nothrow__));
# 580 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern int pthread_once (pthread_once_t *__once_control,
    void (*__init_routine) (void));






extern int pthread_setcancelstate (int __state, int *__oldstate);



extern int pthread_setcanceltype (int __type, int *__oldtype);


extern int pthread_cancel (pthread_t __cancelthread);




extern void pthread_testcancel (void);
# 614 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern void _pthread_cleanup_push (struct _pthread_cleanup_buffer *__buffer,
       void (*__routine) (void *),
       void *__arg) __attribute__ ((__nothrow__));







extern void _pthread_cleanup_pop (struct _pthread_cleanup_buffer *__buffer,
      int __execute) __attribute__ ((__nothrow__));
# 635 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern void _pthread_cleanup_push_defer (struct _pthread_cleanup_buffer *__buffer,
      void (*__routine) (void *),
      void *__arg) __attribute__ ((__nothrow__));
extern void __pthread_cleanup_push_defer (struct _pthread_cleanup_buffer *__buffer,
       void (*__routine) (void *),
       void *__arg) __attribute__ ((__nothrow__));
# 649 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern void _pthread_cleanup_pop_restore (struct _pthread_cleanup_buffer *__buffer,
       int __execute) __attribute__ ((__nothrow__));
extern void __pthread_cleanup_pop_restore (struct _pthread_cleanup_buffer *__buffer,
        int __execute) __attribute__ ((__nothrow__));
# 668 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/sigthread.h" 1
# 31 "/home/cem/ubicom/uClinux/uClibc/include/bits/sigthread.h"
extern int pthread_sigmask (int __how,
       __const __sigset_t *__restrict __newmask,
       __sigset_t *__restrict __oldmask)__attribute__ ((__nothrow__));


extern int pthread_kill (pthread_t __threadid, int __signo) __attribute__ ((__nothrow__));
# 669 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h" 2
# 684 "/home/cem/ubicom/uClinux/uClibc/include/pthread.h"
extern int pthread_atfork (void (*__prepare) (void),
      void (*__parent) (void),
      void (*__child) (void)) __attribute__ ((__nothrow__));




extern void pthread_kill_other_threads_np (void) __attribute__ ((__nothrow__));


# 16 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_mutex.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_pthread.h" 1
# 17 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_mutex.h" 2
# 120 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h" 2
# 170 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
typedef struct {
 __off_t __pos;






} __STDIO_fpos_t;


typedef struct {
 __off64_t __pos;






} __STDIO_fpos64_t;




typedef __off64_t __offmax_t;
# 233 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
struct __STDIO_FILE_STRUCT {
 unsigned short __modeflags;







 unsigned char __ungot[2];

 int __filedes;

 unsigned char *__bufstart;
 unsigned char *__bufend;
 unsigned char *__bufpos;
 unsigned char *__bufread;


 unsigned char *__bufgetc_u;


 unsigned char *__bufputc_u;





 struct __STDIO_FILE_STRUCT *__nextopen;
# 277 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
 int __user_locking;
 pthread_mutex_t __lock;





};
# 384 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
extern int __fgetc_unlocked(FILE *__stream);
extern int __fputc_unlocked(int __c, FILE *__stream);
# 404 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
extern FILE *__stdin;
# 463 "/home/cem/ubicom/uClinux/uClibc/include/bits/uClibc_stdio.h"
extern FILE *__stdout;
# 73 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2



# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stdarg.h" 1 3 4
# 43 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 77 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2






typedef __STDIO_fpos64_t fpos_t;



typedef __STDIO_fpos64_t fpos64_t;
# 131 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/stdio_lim.h" 1
# 132 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h" 2



extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;







extern int remove (__const char *__filename) __attribute__ ((__nothrow__));

extern int rename (__const char *__old, __const char *__new) __attribute__ ((__nothrow__));




# 160 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64");






extern FILE *tmpfile64 (void);



extern char *tmpnam (char *__s) __attribute__ ((__nothrow__));





extern char *tmpnam_r (char *__s) __attribute__ ((__nothrow__));
# 189 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern char *tempnam (__const char *__dir, __const char *__pfx)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__));








extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 214 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fflush_unlocked (FILE *__stream);
# 224 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fcloseall (void);




# 245 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern FILE *fopen (__const char *__restrict __filename, __const char *__restrict __modes) __asm__ ("" "fopen64");

extern FILE *freopen (__const char *__restrict __filename, __const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64");









extern FILE *fopen64 (__const char *__restrict __filename,
        __const char *__restrict __modes);
extern FILE *freopen64 (__const char *__restrict __filename,
   __const char *__restrict __modes,
   FILE *__restrict __stream);




extern FILE *fdopen (int __fd, __const char *__modes) __attribute__ ((__nothrow__));
# 289 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"



extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__));





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__));








extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));






extern int vasprintf (char **__restrict __ptr, __const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));





extern int asprintf (char **__restrict __ptr,
       __const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));







extern int vdprintf (int __fd, __const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, __const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));








extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...);




extern int scanf (__const char *__restrict __format, ...);

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__));








extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0)));





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0)));


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__scanf__, 2, 0)));









extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 451 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 465 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fgetc_unlocked (FILE *__stream);











extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 498 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);
# 517 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);








extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream);






extern char *gets (char *__s);

# 546 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream);
# 567 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern __ssize_t getdelim (char **__restrict __lineptr,
        size_t *__restrict __n, int __delimiter,
        FILE *__restrict __stream);







extern __ssize_t getline (char **__restrict __lineptr,
       size_t *__restrict __n,
       FILE *__restrict __stream);








extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream);




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s);

# 625 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fputs_unlocked (__const char *__restrict __s,
      FILE *__restrict __stream);
# 636 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream);
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream);








extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream);




extern void rewind (FILE *__stream);

# 680 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64");


extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64");








# 705 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64");

extern int fsetpos (FILE *__stream, __const fpos_t *__pos) __asm__ ("" "fsetpos64");









extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
extern __off64_t ftello64 (FILE *__stream);
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos);
extern int fsetpos64 (FILE *__stream, __const fpos64_t *__pos);




extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__));




extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__));








extern void perror (__const char *__s);

# 760 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern int fileno (FILE *__stream) __attribute__ ((__nothrow__));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
# 775 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern FILE *popen (__const char *__command, __const char *__modes);





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) __attribute__ ((__nothrow__));





extern char *cuserid (char *__s);
# 815 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__));
# 869 "/home/cem/ubicom/uClinux/uClibc/include/stdio.h"

# 34 "nsswitch/winbind_nss_config.h" 2
# 75 "nsswitch/winbind_nss_config.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 1
# 29 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"






typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;




typedef __loff_t loff_t;





typedef __ino64_t ino_t;




typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;
# 105 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"
typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;
# 137 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"
typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;





# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 1 3 4
# 148 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 2



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
# 195 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int int16_t __attribute__ ((__mode__ (__HI__)));
typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int int64_t __attribute__ ((__mode__ (__DI__)));


typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));

typedef int register_t __attribute__ ((__mode__ (__word__)));
# 217 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/endian.h" 1
# 37 "/home/cem/ubicom/uClinux/uClibc/include/endian.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/endian.h" 1
# 38 "/home/cem/ubicom/uClinux/uClibc/include/endian.h" 2
# 218 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 2


# 1 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h" 1
# 31 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/select.h" 1
# 32 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h" 2


# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/sigset.h" 1
# 35 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h" 2
# 46 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/time.h" 1
# 73 "/home/cem/ubicom/uClinux/uClibc/include/bits/time.h"
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 47 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h" 2
# 55 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
typedef long int __fd_mask;
# 67 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 99 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"

# 109 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 121 "/home/cem/ubicom/uClinux/uClibc/include/sys/select.h"
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);



# 221 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 2


# 1 "/home/cem/ubicom/uClinux/uClibc/include/sys/sysmacros.h" 1
# 29 "/home/cem/ubicom/uClinux/uClibc/include/sys/sysmacros.h"
__extension__
static __inline unsigned int gnu_dev_major (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__
static __inline unsigned int gnu_dev_minor (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__
static __inline unsigned long long int gnu_dev_makedev (unsigned int __major,
       unsigned int __minor)
     __attribute__ ((__nothrow__));


__extension__ static __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_major (unsigned long long int __dev)
{
  return ((__dev >> 8) & 0xfff) | ((unsigned int) (__dev >> 32) & ~0xfff);
}

__extension__ static __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_minor (unsigned long long int __dev)
{
  return (__dev & 0xff) | ((unsigned int) (__dev >> 12) & ~0xff);
}

__extension__ static __inline unsigned long long int
__attribute__ ((__nothrow__)) gnu_dev_makedev (unsigned int __major, unsigned int __minor)
{
  return ((__minor & 0xff) | ((__major & 0xfff) << 8)
   | (((unsigned long long int) (__minor & ~0xff)) << 12)
   | (((unsigned long long int) (__major & ~0xfff)) << 32));
}
# 224 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 2




typedef __blksize_t blksize_t;
# 248 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h"
typedef __blkcnt64_t blkcnt_t;



typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;





# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/pthreadtypes.h" 1
# 271 "/home/cem/ubicom/uClinux/uClibc/include/sys/types.h" 2



# 76 "nsswitch/winbind_nss_config.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h" 1
# 103 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"


# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/stat.h" 1
# 36 "/home/cem/ubicom/uClinux/uClibc/include/bits/stat.h"
struct stat
  {
    __dev_t st_dev;




    unsigned int __pad1;
    __ino_t __st_ino;

    __mode_t st_mode;
    __nlink_t st_nlink;
    __uid_t st_uid;
    __gid_t st_gid;
    __dev_t st_rdev;




    unsigned int __pad2;
    __off64_t st_size;

    __blksize_t st_blksize;




    __blkcnt64_t st_blocks;
# 79 "/home/cem/ubicom/uClinux/uClibc/include/bits/stat.h"
    __time_t st_atime;
    unsigned long int st_atimensec;
    __time_t st_mtime;
    unsigned long int st_mtimensec;
    __time_t st_ctime;
    unsigned long int st_ctimensec;





    __ino64_t st_ino;

  };


struct stat64
  {
    __dev_t st_dev;
    unsigned int __pad1;

    __ino_t __st_ino;
    __mode_t st_mode;
    __nlink_t st_nlink;
    __uid_t st_uid;
    __gid_t st_gid;
    __dev_t st_rdev;
    unsigned int __pad2;
    __off64_t st_size;
    __blksize_t st_blksize;

    __blkcnt64_t st_blocks;
# 122 "/home/cem/ubicom/uClinux/uClibc/include/bits/stat.h"
    __time_t st_atime;
    unsigned long int st_atimensec;
    __time_t st_mtime;
    unsigned long int st_mtimensec;
    __time_t st_ctime;
    unsigned long int st_ctimensec;

    __ino64_t st_ino;
  };
# 106 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h" 2
# 215 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int stat (__const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__))
     __attribute__ ((__nonnull__ (2)));






extern int stat64 (__const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));
# 263 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int lstat (__const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__))


     __attribute__ ((__nonnull__ (1, 2)));





extern int lstat64 (__const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));





extern int chmod (__const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 293 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__));
# 307 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__));
# 316 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int mkdir (__const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 331 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int mknod (__const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 345 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"
extern int mkfifo (__const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 363 "/home/cem/ubicom/uClinux/uClibc/include/sys/stat.h"

# 77 "nsswitch/winbind_nss_config.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/errno.h" 1
# 32 "/home/cem/ubicom/uClinux/uClibc/include/errno.h"




# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/errno.h" 1
# 25 "/home/cem/ubicom/uClinux/uClibc/include/bits/errno.h"
# 1 "/home/cem/ubicom/uClinux/uClibc/include/bits/errno_values.h" 1
# 26 "/home/cem/ubicom/uClinux/uClibc/include/bits/errno.h" 2
# 43 "/home/cem/ubicom/uClinux/uClibc/include/bits/errno.h"
extern int *__errno_location (void) __attribute__ ((__nothrow__)) __attribute__ ((__const__));
# 37 "/home/cem/ubicom/uClinux/uClibc/include/errno.h" 2
# 59 "/home/cem/ubicom/uClinux/uClibc/include/errno.h"

# 73 "/home/cem/ubicom/uClinux/uClibc/include/errno.h"
typedef int error_t;
# 78 "nsswitch/winbind_nss_config.h" 2
# 1 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h" 1
# 28 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"





# 1 "/home/cem/ubicom/ubicom32tools/bin/../lib/gcc/ubicom32-elf/4.4.0/include/stddef.h" 1 3 4
# 34 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h" 2
# 50 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
struct passwd
{
  char *pw_name;
  char *pw_passwd;
  __uid_t pw_uid;
  __gid_t pw_gid;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};
# 73 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
extern void setpwent (void);





extern void endpwent (void);





extern struct passwd *getpwent (void);
# 95 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
extern struct passwd *fgetpwent (FILE *__stream);







extern int putpwent (__const struct passwd *__restrict __p,
       FILE *__restrict __f);






extern struct passwd *getpwuid (__uid_t __uid);





extern struct passwd *getpwnam (__const char *__name);
# 140 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
extern int getpwent_r (struct passwd *__restrict __resultbuf,
         char *__restrict __buffer, size_t __buflen,
         struct passwd **__restrict __result);


extern int getpwuid_r (__uid_t __uid,
         struct passwd *__restrict __resultbuf,
         char *__restrict __buffer, size_t __buflen,
         struct passwd **__restrict __result);

extern int getpwnam_r (__const char *__restrict __name,
         struct passwd *__restrict __resultbuf,
         char *__restrict __buffer, size_t __buflen,
         struct passwd **__restrict __result);
# 164 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
extern int fgetpwent_r (FILE *__restrict __stream,
   struct passwd *__restrict __resultbuf,
   char *__restrict __buffer, size_t __buflen,
   struct passwd **__restrict __result);
# 181 "/home/cem/ubicom/uClinux/uClibc/include/pwd.h"
extern int getpw (__uid_t __uid, char *__buffer);



# 79 "nsswitch/winbind_nss_config.h" 2
# 1 "./nsswitch/winbind_nss.h" 1
# 61 "./nsswitch/winbind_nss.h"
typedef enum
{
  NSS_STATUS_SUCCESS=0,
  NSS_STATUS_NOTFOUND=1,
  NSS_STATUS_UNAVAIL=2,
  NSS_STATUS_TRYAGAIN=3
} NSS_STATUS;
# 80 "nsswitch/winbind_nss_config.h" 2
# 88 "nsswitch/winbind_nss_config.h"
typedef char pstring[1024];
typedef char fstring[256];







typedef int BOOL;
# 2 "nsswitch/winbind_client.h" 2
# 1 "nsswitch/winbindd_nss.h" 1
# 32 "nsswitch/winbindd_nss.h"
enum winbindd_cmd {

 WINBINDD_INTERFACE_VERSION,



 WINBINDD_GETPWNAM,
 WINBINDD_GETPWUID,
 WINBINDD_GETGRNAM,
 WINBINDD_GETGRGID,
 WINBINDD_GETGROUPS,



 WINBINDD_SETPWENT,
 WINBINDD_ENDPWENT,
 WINBINDD_GETPWENT,
 WINBINDD_SETGRENT,
 WINBINDD_ENDGRENT,
 WINBINDD_GETGRENT,



 WINBINDD_PAM_AUTH,
 WINBINDD_PAM_AUTH_CRAP,
 WINBINDD_PAM_CHAUTHTOK,



 WINBINDD_LIST_USERS,
 WINBINDD_LIST_GROUPS,
 WINBINDD_LIST_TRUSTDOM,



 WINBINDD_LOOKUPSID,
 WINBINDD_LOOKUPNAME,



 WINBINDD_SID_TO_UID,
 WINBINDD_SID_TO_GID,
 WINBINDD_UID_TO_SID,
 WINBINDD_GID_TO_SID,
 WINBINDD_ALLOCATE_RID,



 WINBINDD_CHECK_MACHACC,
 WINBINDD_PING,
 WINBINDD_INFO,
 WINBINDD_DOMAIN_NAME,

 WINBINDD_DOMAIN_INFO,


 WINBINDD_SHOW_SEQUENCE,



 WINBINDD_WINS_BYIP,
 WINBINDD_WINS_BYNAME,



 WINBINDD_CREATE_USER,
 WINBINDD_CREATE_GROUP,
 WINBINDD_ADD_USER_TO_GROUP,
 WINBINDD_REMOVE_USER_FROM_GROUP,
 WINBINDD_SET_USER_PRIMARY_GROUP,
 WINBINDD_DELETE_USER,
 WINBINDD_DELETE_GROUP,


 WINBINDD_GETGRLST,

 WINBINDD_NETBIOS_NAME,


 WINBINDD_PRIV_PIPE_DIR,


 WINBINDD_GETUSERSIDS,


 WINBINDD_NUM_CMDS
};

typedef struct winbindd_pw {
 fstring pw_name;
 fstring pw_passwd;
 uid_t pw_uid;
 gid_t pw_gid;
 fstring pw_gecos;
 fstring pw_dir;
 fstring pw_shell;
} WINBINDD_PW;


typedef struct winbindd_gr {
 fstring gr_name;
 fstring gr_passwd;
 gid_t gr_gid;
 int num_gr_mem;
 int gr_mem_ofs;
 char **gr_mem;
} WINBINDD_GR;
# 154 "nsswitch/winbindd_nss.h"
struct winbindd_request {
 uint32 length;
 enum winbindd_cmd cmd;
 pid_t pid;
 uint32 flags;
 fstring domain_name;

 union {
  fstring winsreq;
  fstring username;
  fstring groupname;
  uid_t uid;
  gid_t gid;
  struct {



   fstring user;
   fstring pass;
          fstring require_membership_of_sid;
  } auth;
                struct {
                        unsigned char chal[8];
                        fstring user;
                        fstring domain;
                        fstring lm_resp;
                        unsigned short lm_resp_len;
                        fstring nt_resp;
                        unsigned short nt_resp_len;
   fstring workstation;
          fstring require_membership_of_sid;
                } auth_crap;
                struct {
                    fstring user;
                    fstring oldpass;
                    fstring newpass;
                } chauthtok;
  fstring sid;
  struct {
   fstring dom_name;
   fstring name;
  } name;
  uint32 num_entries;
  struct {
   fstring username;
   fstring groupname;
  } acct_mgt;
 } data;
 char null_term;
};



enum winbindd_result {
 WINBINDD_ERROR,
 WINBINDD_OK
};



struct winbindd_response {



 uint32 length;
 enum winbindd_result result;



 union {
  int interface_version;

  fstring winsresp;



  struct winbindd_pw pw;



  struct winbindd_gr gr;

  uint32 num_entries;
  struct winbindd_sid {
   fstring sid;
   int type;
  } sid;
  struct winbindd_name {
   fstring dom_name;
   fstring name;
   int type;
  } name;
  uid_t uid;
  gid_t gid;
  struct winbindd_info {
   char winbind_separator;
   fstring samba_version;
  } info;
  fstring domain_name;
  fstring netbios_name;

  struct auth_reply {
   uint32 nt_status;
   fstring nt_status_string;
   fstring error_string;
   int pam_error;
   char user_session_key[16];
   char first_8_lm_hash[8];
  } auth;
  uint32 rid;
  struct {
   fstring name;
   fstring alt_name;
   fstring sid;
   BOOL native_mode;
   BOOL active_directory;
   BOOL primary;
   uint32 sequence_number;
  } domain_info;
 } data;



 void *extra_data;
};
# 3 "nsswitch/winbind_client.h" 2

void init_request(struct winbindd_request *req,int rq_type);
NSS_STATUS winbindd_send_request(int req_type,
     struct winbindd_request *request);
NSS_STATUS winbindd_get_response(struct winbindd_response *response);
NSS_STATUS winbindd_request(int req_type,
       struct winbindd_request *request,
       struct winbindd_response *response);
int winbind_open_pipe_sock(void);
int write_sock(void *buffer, int count);
int read_reply(struct winbindd_response *response);
void close_sock(void);
void free_response(struct winbindd_response *response);
# 28 "nsswitch/wb_common.c" 2



int winbindd_fd = -1;



void free_response(struct winbindd_response *response)
{


 if (response)
  do { if(response->extra_data) {free(response->extra_data); response->extra_data=((void *)0);} } while(0);
}



void init_request(struct winbindd_request *request, int request_type)
{
 request->length = sizeof(struct winbindd_request);

 request->cmd = (enum winbindd_cmd)request_type;
 request->pid = getpid();

}



void init_response(struct winbindd_response *response)
{


 response->result = WINBINDD_ERROR;
}



void close_sock(void)
{
 if (winbindd_fd != -1) {
  close(winbindd_fd);
  winbindd_fd = -1;
 }
}
# 80 "nsswitch/wb_common.c"
static int make_nonstd_fd_internals(int fd, int limit )
{
 int new_fd;
 if (fd >= 0 && fd <= 2) {
# 96 "nsswitch/wb_common.c"
  if (limit <= 0)
   return -1;

  new_fd = dup(fd);
  if (new_fd == -1)
   return -1;


  new_fd = make_nonstd_fd_internals(new_fd, limit - 1);
  close(fd);
  return new_fd;

 }
 return fd;
}
# 120 "nsswitch/wb_common.c"
static int make_safe_fd(int fd)
{
 int result, flags;
 int new_fd = make_nonstd_fd_internals(fd, 3);
 if (new_fd == -1) {
  close(fd);
  return -1;
 }
# 140 "nsswitch/wb_common.c"
 if ((flags = fcntl(new_fd, F_GETFL)) == -1) {
  close(new_fd);
  return -1;
 }

 flags |= FNDELAY;
 if (fcntl(new_fd, F_SETFL, flags) == -1) {
  close(new_fd);
  return -1;
 }
# 165 "nsswitch/wb_common.c"
 return new_fd;
}



static int winbind_named_pipe_sock(const char *dir)
{
 struct sockaddr_un sunaddr;
 struct stat st;
 pstring path;
 int fd;
 int wait_time;
 int slept;



 if (lstat(dir, &st) == -1) {
  return -1;
 }

 if (!((((st.st_mode)) & 0170000) == (0040000)) ||
     (st.st_uid != 0 && st.st_uid != geteuid())) {
  return -1;
 }



 strncpy(path, dir, sizeof(path) - 1);
 path[sizeof(path) - 1] = '\0';

 strncat(path, "/", sizeof(path) - 1 - strlen(path));
 path[sizeof(path) - 1] = '\0';

 strncat(path, "pipe", sizeof(path) - 1 - strlen(path));
 path[sizeof(path) - 1] = '\0';

 memset((char *)&(sunaddr), 0, sizeof(sunaddr));
 sunaddr.sun_family = AF_UNIX;
 strncpy(sunaddr.sun_path, path, sizeof(sunaddr.sun_path) - 1);





 if (lstat(path, &st) == -1) {
  return -1;
 }



 if (!((((st.st_mode)) & 0170000) == (0140000)) ||
     (st.st_uid != 0 && st.st_uid != geteuid())) {
  return -1;
 }



 if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
  return -1;
 }



 if ((fd = make_safe_fd( fd)) == -1) {
  return fd;
 }

 for (wait_time = 0; connect(fd, (struct sockaddr *)&sunaddr, sizeof(sunaddr)) == -1;
   wait_time += slept) {
  struct timeval tv;
  fd_set w_fds;
  int ret;
  int connect_errno = 0, errnosize;

  if (wait_time >= 30)
   goto error_out;

  switch ((*__errno_location ())) {
   case 115:
    do { unsigned int __i; fd_set *__arr = (&w_fds); for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i) ((__arr)->fds_bits)[__i] = 0; } while (0);
    (((&w_fds)->fds_bits)[((fd) / (8 * sizeof (__fd_mask)))] |= ((__fd_mask) 1 << ((fd) % (8 * sizeof (__fd_mask)))));
    tv.tv_sec = 30 - wait_time;
    tv.tv_usec = 0;

    ret = select(fd + 1, ((void *)0), &w_fds, ((void *)0), &tv);

    if (ret > 0) {
     errnosize = sizeof(connect_errno);

     ret = getsockopt(fd, SOL_SOCKET,
       SO_ERROR, &connect_errno, &errnosize);

     if (ret >= 0 && connect_errno == 0) {

      goto out;
     }
    }

    slept = 30;
    break;
   case 11:
    slept = rand() % 3 + 1;
    sleep(slept);
    break;
   default:
    goto error_out;
  }

 }

  out:

 return fd;

  error_out:

 close(fd);
 return -1;

 if (connect(fd, (struct sockaddr *)&sunaddr,
      sizeof(sunaddr)) == -1) {
  close(fd);
  return -1;
 }

 return fd;
}



int winbind_open_pipe_sock(void)
{
# 338 "nsswitch/wb_common.c"
 return -1;

}



int write_sock(void *buffer, int count)
{
 int result, nwritten;



 restart:

 if (winbind_open_pipe_sock() == -1) {
  return -1;
 }



 nwritten = 0;

 while(nwritten < count) {
  struct timeval tv;
  fd_set r_fds;




  do { unsigned int __i; fd_set *__arr = (&r_fds); for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i) ((__arr)->fds_bits)[__i] = 0; } while (0);
  (((&r_fds)->fds_bits)[((winbindd_fd) / (8 * sizeof (__fd_mask)))] |= ((__fd_mask) 1 << ((winbindd_fd) % (8 * sizeof (__fd_mask)))));
  memset((char *)&(tv), 0, sizeof(tv));

  if (select(winbindd_fd + 1, &r_fds, ((void *)0), ((void *)0), &tv) == -1) {
   close_sock();
   return -1;
  }



  if (!((((&r_fds)->fds_bits)[((winbindd_fd) / (8 * sizeof (__fd_mask)))] & ((__fd_mask) 1 << ((winbindd_fd) % (8 * sizeof (__fd_mask))))) != 0)) {



   result = write(winbindd_fd,
           (char *)buffer + nwritten,
           count - nwritten);

   if ((result == -1) || (result == 0)) {



    close_sock();
    return -1;
   }

   nwritten += result;

  } else {



   close_sock();
   goto restart;
  }
 }

 return nwritten;
}



static int read_sock(void *buffer, int count)
{
 int result = 0, nread = 0;
 int total_time = 0, selret;


 while(nread < count) {
  struct timeval tv;
  fd_set r_fds;




  do { unsigned int __i; fd_set *__arr = (&r_fds); for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i) ((__arr)->fds_bits)[__i] = 0; } while (0);
  (((&r_fds)->fds_bits)[((winbindd_fd) / (8 * sizeof (__fd_mask)))] |= ((__fd_mask) 1 << ((winbindd_fd) % (8 * sizeof (__fd_mask)))));
  memset((char *)&(tv), 0, sizeof(tv));

  tv.tv_sec = 5;

  if ((selret = select(winbindd_fd + 1, &r_fds, ((void *)0), ((void *)0), &tv)) == -1) {
   close_sock();
   return -1;
  }

  if (selret == 0) {

   if (total_time >= 30) {

    close_sock();
    return -1;
   }
   total_time += 5;
   continue;
  }

  if (((((&r_fds)->fds_bits)[((winbindd_fd) / (8 * sizeof (__fd_mask)))] & ((__fd_mask) 1 << ((winbindd_fd) % (8 * sizeof (__fd_mask))))) != 0)) {



   result = read(winbindd_fd, (char *)buffer + nread,
         count - nread);

   if ((result == -1) || (result == 0)) {





    close_sock();
    return -1;
   }

   nread += result;

  }
 }

 return result;
}



int read_reply(struct winbindd_response *response)
{
 int result1, result2 = 0;

 if (!response) {
  return -1;
 }



 if ((result1 = read_sock(response, sizeof(struct winbindd_response)))
     == -1) {

  return -1;
 }





 response->extra_data = ((void *)0);



 if (response->length > sizeof(struct winbindd_response)) {
  int extra_data_len = response->length -
   sizeof(struct winbindd_response);



  if (!(response->extra_data = malloc(extra_data_len))) {
   return -1;
  }

  if ((result2 = read_sock(response->extra_data, extra_data_len))
      == -1) {
   free_response(response);
   return -1;
  }
 }



 return result1 + result2;
}





NSS_STATUS winbindd_send_request(int req_type, struct winbindd_request *request)
{
 struct winbindd_request lrequest;
 char *env;
 int value;



 if ( (env = getenv("_NO_WINBINDD")) != ((void *)0) ) {
  value = atoi(env);
  if ( value == 1 )
   return NSS_STATUS_NOTFOUND;
 }

 if (!request) {
  memset((char *)&(lrequest), 0, sizeof(lrequest));
  request = &lrequest;
 }



 init_request(request, req_type);

 if (write_sock(request, sizeof(*request)) == -1) {
  return NSS_STATUS_UNAVAIL;
 }

 return NSS_STATUS_SUCCESS;
}





NSS_STATUS winbindd_get_response(struct winbindd_response *response)
{
 struct winbindd_response lresponse;

 if (!response) {
  memset((char *)&(lresponse), 0, sizeof(lresponse));
  response = &lresponse;
 }

 init_response(response);


 if (read_reply(response) == -1) {
  return NSS_STATUS_UNAVAIL;
 }


 if (response == &lresponse) {
  free_response(response);
 }


 if (response->result != WINBINDD_OK) {
  return NSS_STATUS_NOTFOUND;
 }

 return NSS_STATUS_SUCCESS;
}



NSS_STATUS winbindd_request(int req_type,
       struct winbindd_request *request,
       struct winbindd_response *response)
{
 NSS_STATUS status;

 status = winbindd_send_request(req_type, request);
 if (status != NSS_STATUS_SUCCESS)
  return(status);
 return winbindd_get_response(response);
}
# 607 "nsswitch/wb_common.c"
BOOL winbind_off( void )
{
 static char *s = "_NO_WINBINDD" "=1";

 return putenv(s) != -1;
}

BOOL winbind_on( void )
{
 static char *s = "_NO_WINBINDD" "=0";

 return putenv(s) != -1;
}
