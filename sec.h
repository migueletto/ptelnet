#ifdef UNIX
#define SEC(name)  
#else
#define SEC(name) __attribute__ ((section (name)))
#endif
