#ifndef _NET_CACHE_H_
#define _NET_CACHE_H_

//#include "_r500_config.h"     // sakaguchi 2021.07.20
#include "MyDefine.h"           // sakaguchi 2021.07.20
#include "Config.h"             // sakaguchi 2021.07.20

#include "stdint.h"
#include "stdbool.h"

#ifdef EDF
#undef EDF
#endif

#ifdef _NET_CACHE_C_
#define EDF
#else
#define EDF extern
#endif


EDF void dns_local_cache_init(void);
EDF void dns_local_cache_put(char *name, uint32_t ipv4Addr);
EDF uint32_t dns_local_cache_get(char *name);

EDF void dns_local_cache_test(void);


#endif /* _NET_CACHE_H_ */
