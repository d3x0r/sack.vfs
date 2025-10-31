
#ifdef offsetof
#  undef offsetof
#  define offsetof( a, b ) ( &((a *)(0))->b )
#endif
