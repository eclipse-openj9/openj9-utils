#include <stdlib.h>
#include <stdio.h>
#include "utils.hpp"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}