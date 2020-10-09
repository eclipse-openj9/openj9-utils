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


json perfProcess(pid_t processID, int recordTime) {
    /* Perf process runs perf tool to collect perf data of given process.
    * Inputs:	pid_t 	processID:	process ID of running application.
    * Outputs:	json	perf_data:	perf data collected from application.
    * */
    char* pidStr = (char*)std::to_string(processID).c_str();
    char *args[3] = {"/usr/bin/perf", "record", NULL};

    // Change to /tmp folder to store the perf data
    if (chdir("/tmp") == -1) {
        perror("chdir");
        _exit(1);
    }

    pid_t pid;
    int fd, status;

    if ((pid = fork()) == -1) {
        perror("fork");
        _exit(1);
    }

    if (pid == 0) {
        // Run perf record to collect process data into perf.data
        if (execv(args[0], args) == -1) {
            perror("execv");
            _exit(1);
        }
    }

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
        std::string progExpression ("\\s+([^\\s]+)");
        std::string pidExpression ("\\s+([^\\s]+)");
        std::string cpuExpression ("\\s+([^\\s]+)");
        std::string timeExpression ("\\s+([^\\s]+):");
        std::string cyclesExpression ("\\s+([^\\s]+\\s+)cycles:");
        std::string addressExpression ("\\s+([^\\s]+)");
        std::string instructionExpression ("\\s+([^\\s]+)");
        std::string pathExpression ("\\s+([^\\s]+)");

        std::regex expression (progExpression + pidExpression + cpuExpression + timeExpression + cyclesExpression + addressExpression + instructionExpression + pathExpression);

        if (std::regex_search(lineStr, matches, expression)) {
            // Put into JSON object
            std::string idStr = std::to_string(idCount); // define unique id for each line
            perfData[idStr]["prog"] = matches[1].str().c_str();
            perfData[idStr]["pid"] = matches[2].str().c_str();
            perfData[idStr]["cpu"] = matches[3].str().c_str();
            perfData[idStr]["time"] = matches[4].str().c_str();
            perfData[idStr]["cycles"] = matches[5].str().c_str();
            perfData[idStr]["address"] = matches[6].str().c_str();
            perfData[idStr]["instruction"] = matches[7].str().c_str();
            perfData[idStr]["path"] = matches[8].str().c_str();
            perfData[idStr]["record"] = lineStr.c_str();
            idCount++;
        }

    }
    return perfData;
}
