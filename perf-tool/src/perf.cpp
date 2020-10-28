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
#include <string>


const perfFieldRegex mapRegex[PERF_FIELD_MAX] = {
    {"unknown", "(.*)"},                        // PERF_FIELD_UNKNOWN
    {"prog", "\\s+(.+)[\\s]{2,}"},              // PERF_FIELD_PROG
    {"pid", "\\s*([0-9]+)"},                    // PERF_FIELD_PID
    {"cpu", "\\s+([^\\s]+)"},                   // PERF_FIELD_CPU
    {"time", "\\s+([^\\s]+):"},                 // PERF_FIELD_TIME
    {"cycles", "\\s+([^\\s]+\\s+)cycles:"},     // PERF_FIELD_CYCLES
    {"address", "\\s+([^\\s]+)"},               // PERF_FIELD_ADDRESS
    {"instruction", "\\s+([^\\s]+)"},           // PERF_FIELD_INSTRUCTION
    {"path", "\\s+([^\\s]+)"},                  // PERF_FIELD_PATH
};


json perfProcess(pid_t processID, int recordTime) {
    /* Perf process runs perf tool to collect perf data of given process.
    * Inputs:	pid_t 	processID:	process ID of running application.
    * Outputs:	json	perf_data:	perf data collected from application.
    * */

    pid_t pid;
    int fd, status;
    std::string pidStr = std::to_string(processID); // define unique id for each line
    char* pidCharArr = (char*)pidStr.c_str();
    char* perfPath = (char*)"/usr/bin/perf";
    char* perfRecord = (char*)"record";
    char* perfPidFlag = (char*)"-p";
    char *args[5] = {perfPath, perfRecord, perfPidFlag, pidCharArr, NULL};

    // Change to /tmp folder to store the perf data
    if (chdir("/tmp") == -1) {
        perror("chdir");
        _exit(1);
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        _exit(1);
    }

    if (pid == 0) {
        // Child process: Run perf record to collect process data into perf.data
        if (execv(args[0], args) == -1) {
            perror("execv");
            _exit(1);
        }
    }

    // Parent process sleeps for recordTime before sending terminate signal to child
    sleep(recordTime);
    kill(pid, SIGTERM);
    pid = wait(&status);
    system("perf script > perf.data.txt");

    // Save into json format
    json perfData;

    std::ifstream file("perf.data.txt");
    std::string lineStr;
    int idCount = 0;
    while (std::getline(file, lineStr)) {
        // Process str
        // Parse perf.data.txt using regex to get data for each process
        std::smatch matches;

        // To-do: make this into string array that is indexed by enum (enum containing options)
        std::string progExpression ("\\s+(.+)[\\s]{2,}");
        std::string pidExpression ("\\s*([0-9]+)");
        std::string cpuExpression ("\\s+([^\\s]+)");
        std::string timeExpression ("\\s+([^\\s]+):");
        std::string cyclesExpression ("\\s+([^\\s]+)\\s+cycles:");
        std::string addressExpression ("\\s+([^\\s]+)");
        std::string instructionExpression ("\\s+([^\\s]+)");
        std::string pathExpression ("\\s+([^\\s]+)");

        std::regex expression (progExpression + pidExpression + timeExpression + cyclesExpression + addressExpression + instructionExpression + pathExpression + "$");

        if (std::regex_search(lineStr, matches, expression)) {
            // Put into JSON object
            std::string idStr = std::to_string(idCount); // define unique id for each line
            perfData[idStr]["prog"] = matches[1].str().c_str();
            perfData[idStr]["pid"] = matches[2].str().c_str();
            perfData[idStr]["time"] = matches[3].str().c_str();
            perfData[idStr]["cycles"] = matches[4].str().c_str();
            perfData[idStr]["address"] = matches[5].str().c_str();
            perfData[idStr]["instruction"] = matches[6].str().c_str();
            perfData[idStr]["path"] = matches[7].str().c_str();
            perfData[idStr]["record"] = lineStr.c_str();
            idCount++;
        }

    }
    return perfData;
}
