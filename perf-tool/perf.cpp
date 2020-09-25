#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <perf.hpp>



json perfProcess(pid_t processID) {
	/* Perf process runs perf tool to collect perf data of given process.
	 * Inputs:	pid_t 	processID:	process ID of running application.
	 * Outputs:	json	perf_data:	perf data collected from application.
	 * */
	char* pidStr = (char*)std::to_string(processID).c_str();
	char *args[2] = {pidStr, NULL};
	char *envp[1] = {NULL};

	// Change to /tmp folder to store the perf data
	if (chdir("/tmp") == -1)
		perror("chdir");

	// Run perf record to collect process data into perf.data
	if (execve("perf record -p", args, envp) == -1)
		perror("execve");
	
	// Run perf script to get all data
	char* cmdPerfScript = (char*) "perf script > perf.data.txt";
	char* perfScriptArgs[2] = {cmdPerfScript, NULL};
	if (execve(perfScriptArgs[0], perfScriptArgs, NULL) == -1)
		perror("execve");
	
	// Save into json format
	json perfData;	

	// Parse perf.data.txt using regex to get data for each process
	// Temp dummy variables
	perfData["pid"] = pidStr;
	perfData["overhead"] = "dummyOverhead";
	perfData["command"] = "dummyCommand";
	perfData["sharedObject"] = "dummySharedObject";
	perfData["symbol"] = "dummySymbol";

	printf("should be returned\n");

	return perfData;
}

/*int main(int argc, char* argv[]) {
	json j;
	pid_t pid;
	pid = getpid();
	j = perfProcess(pid);
	std::string str = j.dump();

	printf("%s\n", str.c_str());
	return 0;
}*/
