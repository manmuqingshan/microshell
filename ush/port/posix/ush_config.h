
#ifndef USH_CONFIG_H
#define USH_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#define USH_CONFIG_PATH_MAX_LENGTH     128UL

#define USH_ASSERT(cond) { if (!(cond)) { fprintf(stderr, "ASSERT: %s:%d\n", __FILE__, __LINE__); exit(-1); } }

#ifdef __cplusplus
}
#endif

#endif /* USH_CONFIG_H */
