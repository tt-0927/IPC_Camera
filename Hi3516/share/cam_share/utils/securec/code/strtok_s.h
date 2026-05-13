
#include "securec.h"

static char *SecFindBegin(char *strToken, const char *strDelimit);

static char *SecFindRest(char *strToken, const char *strDelimit);

char *strtok_s(char *strToken, const char *strDelimit, char **context);