#ifndef __PERF_H__
#define __PERF_H__

#include <json.hpp>
#include <sys/types.h>
#include <unistd.h>

using json = nlohmann::json;

json perfProcess(pid_t processID);

#endif
