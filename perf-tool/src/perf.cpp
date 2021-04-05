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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <perf.hpp>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex>
#include <fstream>
#include <infra.hpp>
#include <limits.h>

using namespace std;


/* For use later if user specifies which fields to return */
const perfFieldRegex mapRegex[PERF_FIELD_MAX] = {
    {"unknown", "(.*)"},                        // PERF_FIELD_UNKNOWN
    {"prog", "\\s+(.+)"},                       // PERF_FIELD_PROG
    {"pid", "\\s+([0-9]+)"},                    // PERF_FIELD_PID
    {"cpu", "\\s+([^\\s]+)"},                   // PERF_FIELD_CPU
    {"time", "\\s+([^\\s]+):"},                 // PERF_FIELD_TIME
    {"cycles", "\\s+([^\\s]+)\\s+cycles:"},     // PERF_FIELD_CYCLES
    {"address", "\\s+([^\\s]+)"},               // PERF_FIELD_ADDRESS
    {"instruction", "\\s+([^\\s]+)"},           // PERF_FIELD_INSTRUCTION
    {"path", "\\s+([^\\s]+)"},                  // PERF_FIELD_PATH
};


void perfProcess(pid_t processID, int recordTime) {
    /* Perf process runs perf tool to collect perf data of given process.
    * Inputs:	pid_t 	processID:	process ID of running application.
    * Outputs:	json	perf_data:	perf data collected from application.
    * */

    pid_t pid;
    int fd, status;
    string pidStr = to_string(processID); // define unique id for each line
    char currDir[PATH_MAX];
    char* pidCharArr = (char*)pidStr.c_str();
    char* perfPath = (char*)"/usr/bin/perf";
    char* perfRecord = (char*)"record";
    char* perfPidFlag = (char*)"-p";
    char *args[5] = {perfPath, perfRecord, perfPidFlag, pidCharArr, NULL};

    // Save current directory to return to later
    if (getcwd(currDir, PATH_MAX) == NULL) {
        perror("getcwd");
        return;
    }

    // Change to /tmp folder to store the perf data
    if (chdir("/tmp") == -1) {
        perror("chdir");
        return;
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // Child process: Run perf record to collect process data into perf.data
        if (execv(args[0], args) == -1) {
            perror("execv");
            return;
        }
    }

    // Parent process sleeps for recordTime before sending terminate signal to child
    sleep(recordTime);
    kill(pid, SIGTERM);
    pid = wait(&status);
    system("perf script > perf.data.txt");

    ifstream file("perf.data.txt");
    string lineStr;
    int idCount = 0;
    while (getline(file, lineStr)) {
        // Process str
        // Parse perf.data.txt using regex to get data for each process
        smatch matches;

        // To-do: make this into string array that is indexed by enum (enum containing options)
        string progExpression ("\\s+(.+)");
        string pidExpression ("\\s+([0-9]+)");
        string cpuExpression ("\\s+([^\\s]+)");
        string timeExpression ("\\s+([^\\s]+):");
        string cyclesExpression ("\\s+([^\\s]+)\\s+cycles:");
        string addressExpression ("\\s+([^\\s]+)");
        string instructionExpression ("\\s+([^\\s]+)");
        string pathExpression ("\\s+([^\\s]+)");

        regex expression (progExpression + pidExpression + timeExpression + cyclesExpression + addressExpression + instructionExpression + pathExpression + "$");

        if (regex_search(lineStr, matches, expression)) {
            // Save into json format
            json perfData;

            // Put into JSON object
            string idStr = to_string(idCount); // define unique id for each line
            perfData["id"] = idStr.c_str();
            perfData["prog"] = matches[1].str().c_str();
            perfData["pid"] = matches[2].str().c_str();
            perfData["time"] = matches[3].str().c_str();
            perfData["cycles"] = matches[4].str().c_str();
            perfData["ip"] = matches[5].str().c_str();
            perfData["symbol+offset"] = matches[6].str().c_str();
            perfData["dso"] = matches[7].str().c_str();
            perfData["record"] = lineStr.c_str();
            idCount++;

            sendToServer(perfData, "perfEvent");
        }

    }

    // Restore working directory
    if (chdir(currDir) == -1) {
        perror("chdir");
        return;
    }
    // return perfData;
}
