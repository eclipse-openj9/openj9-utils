/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef __PERF_H__
#define __PERF_H__

#include <json.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <string>

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
