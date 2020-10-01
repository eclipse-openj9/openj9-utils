#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <perf.hpp>
#include <signal.h>


json perfProcess(pid_t processID, int recordTime) {
	/* Perf process runs perf tool to collect perf data of given process.
	 * Inputs:	pid_t 	processID:	process ID of running application.
	 * Outputs:	json	perf_data:	perf data collected from application.
	 * */
	char* pidStr = (char*)std::to_string(processID).c_str();
	char *args[5] = {"/usr/bin/perf", "record", "-p", pidStr,  NULL};
	char *envp[1] = {NULL};
	
	// Make local tmp folder
	if (mkdir("./tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		perror("mkdir");

	// Change to /tmp folder to store the perf data
	if (chdir("./tmp") == -1)
		perror("chdir");
	
	pid_t pid;

	if ((pid = fork()) == -1) {
		perror("fork");
	}

	if (pid == 0) {
		// Run perf record to collect process data into perf.data
		if (execve(args[0], args, envp) == -1)
			perror("execve");
	}
	else {
		sleep(recordTime);
		kill(pid, SIGTERM);
		
		// Run perf script to get all data
		char* cmdPerfScript = (char*) "perf script > perf.data.txt";
		char* perfScriptArgs[5] = {"/usr/bin/perf", "script", ">", "perf.data.txt", NULL};
		//if (execve(perfScriptArgs[0], perfScriptArgs, envp) == -1)
		//	perror("execve");
	
		// Save into json format
		json perfData;	

		// Parse perf.data.txt using regex to get data for each process
		// Temp dummy variables
		perfData["pid"] = pidStr;
		perfData["overhead"] = "dummyOverhead";
		perfData["command"] = "dummyCommand";
		perfData["sharedObject"] = "dummySharedObject";
		perfData["symbol"] = "dummySymbol";

		printf("Returning now\n");

		return perfData;
	}

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
