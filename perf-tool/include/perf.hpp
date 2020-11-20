#ifndef __PERF_H__
#define __PERF_H__

#include <json.hpp>
#include <sys/types.h>
#include <unistd.h>

using json = nlohmann::json;

void perfProcess(pid_t processID, int recordTime);


typedef enum { //to-do: get all options
    PERF_OPTION_UNKNOWN = 0,
    PERF_OPTION_PID,
    PERF_OPTION_REC_TIME,
    PERF_OPTION_FIELDS,
    PERF_OPTION_MAX
} perfOption_t;

typedef enum {
    PERF_FIELD_UNKNOWN = 0,
    PERF_FIELD_PROG,
    PERF_FIELD_PID,
    PERF_FIELD_CPU,
    PERF_FIELD_TIME,
    PERF_FIELD_CYCLES,
    PERF_FIELD_ADDRESS,
    PERF_FIELD_INSTRUCTION,
    PERF_FIELD_PATH,
    PERF_FIELD_MAX
} perfField_t;

typedef struct perfFieldRegex {
    std::string fieldName;
    std::string fieldRegex;
} perfFieldRegex;

extern const perfFieldRegex mapRegex[PERF_FIELD_MAX];

#endif
