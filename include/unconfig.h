/*
 * Inventor/system/inttypes.h and config.h defines some of the same
 * definitions, which causes warnings with a lot of compilers.
 *
 * When including config.h, we therefore undefine those definitions
 * first if they are defined.  The reversed is not necessary, as the
 * inttypes.h file contains the necessary wrapper protection.
 *
 */

#ifdef COIN_CONFIGURE_BUILD
#undef COIN_CONFIGURE_BUILD
#endif

#ifdef COIN_CONFIGURE_HOST
#undef COIN_CONFIGURE_HOST
#endif

#ifdef COIN_CONFIGURE_TARGET
#undef COIN_CONFIGURE_TARGET
#endif

#ifdef HAVE_INTTYPES_H
#undef HAVE_INTTYPES_H
#endif

#ifdef HAVE_STDINT_H
#undef HAVE_STDINT_H
#endif

#ifdef HAVE_SYS_TYPES_H
#undef HAVE_SYS_TYPES_H
#endif

#ifdef HAVE_INT8_T
#undef HAVE_INT8_T
#endif

#ifdef HAVE_UINT8_T
#undef HAVE_UINT8_T
#endif

#ifdef HAVE_INT16_T
#undef HAVE_INT16_T
#endif

#ifdef HAVE_UINT16_T
#undef HAVE_UINT16_T
#endif

#ifdef HAVE_INT32_T
#undef HAVE_INT32_T
#endif

#ifdef HAVE_UINT32_T
#undef HAVE_UINT32_T
#endif

#ifdef HAVE_INT64_T
#undef HAVE_INT64_T
#endif

#ifdef HAVE_UINT64_T
#undef HAVE_UINT64_T
#endif

#ifdef COIN_INT8_T
#undef COIN_INT8_T
#endif

#ifdef COIN_UINT8_T
#undef COIN_UINT8_T
#endif

#ifdef COIN_INT16_T
#undef COIN_INT16_T
#endif

#ifdef COIN_UINT16_T
#undef COIN_UINT16_T
#endif

#ifdef COIN_INT32_T
#undef COIN_INT32_T
#endif

#ifdef COIN_UINT32_T
#undef COIN_UINT32_T
#endif

#ifdef COIN_INT64_T
#undef COIN_INT64_T
#endif

#ifdef COIN_UINT64_T
#undef COIN_UINT64_T
#endif

