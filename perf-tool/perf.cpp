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
			std::string s ("this subject has a submarine as a subsequence");
			std::smatch m;
			std::regex e ("\\b(sub)([^ ]*)");

			std::cout << "Target sequence: " << s << std::endl;
			std::cout << "Regular expression: /\\b(sub)([^ ]*)/" << std::endl;
			std::cout << "The following matches and submatches were found:" << std::endl;

			while (std::regex_search (s,m,e)) {
				s = m.suffix().str();
			}

			// Put into JSON object
			std::string idStr = std::to_string(idCount); // define unique id for each line

			perfData[idStr]["prog"] = idStr.c_str();
			perfData[idStr]["pid"] = idStr.c_str();
			perfData[idStr]["time"] = idStr.c_str();
			perfData[idStr]["cycles"] = idStr.c_str();
			perfData[idStr]["address"] = idStr.c_str();
			perfData[idStr]["instruction"] = idStr.c_str();
			perfData[idStr]["path"] = idStr.c_str();

			idCount++;


			if (idCount > 5) { // for debugging
				break;
			}
  }
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
