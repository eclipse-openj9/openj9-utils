#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <perf.hpp>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>


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
	if (chdir("./tmp") == -1) {
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
		if (execve(args[0], args, envp) == -1) {
			perror("execve");
			_exit(1);
		}
	}
	sleep(recordTime);
	kill(pid, SIGTERM);
	pid = wait(&status);
	system("perf script > perf.data.txt");
	/*
	FILE *fp;
	char path[PATH_MAX];
	char* filename = "perf.data.txt";
        fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	fp = popen("perf script", "r");
	if (fp == NULL) {
		perror("popen");
	}

	while (fgets(path, PATH_MAX, fp) != NULL) {
		write(fd, path, PATH_MAX);
	}

	status = pclose(fp); */
	// to-do: perform error check of status
	
/*
	// Run perf script to get all data
	char* cmdPerfScript = (char*) "perf script > perf.data.txt";
	char* perfScriptArgs[3] = {"/usr/bin/perf", "script", NULL};
		
	// Open file
	char* filename = "perf.data.txt";
	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	if (fd == -1) {
		perror("open");
		_exit(1);
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		perror("dup2");
		close(fd);
		_exit(1);
	}

	if ((pid = fork()) == -1) {
                perror("fork");
                _exit(1);
        }
	
	if (pid == 0) {
		if (execve(perfScriptArgs[0], perfScriptArgs, envp) == -1) {
			perror("execve");
			_exit(1);
		}
	}
	// Else, parent waits for child to return
	pid = wait(&status);
	close(fd);
*/	
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
	
	//close(fd);

	return perfData;
	

}

/*int main(int argc, char* argv[]) {
	json j;
	pid_t pid;
	pid = getpid();
	j = perfProcess(pid, 3);
	std::string str = j.dump();

	printf("%s\n", str.c_str());
	return 0;
}*/
