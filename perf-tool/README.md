# Build Instructions
1. Make sure you have CMake installed & updated. To do so on ubuntu, run `sudo apt-get install cmake`.  
2. Create build directory `mkdir build`, otherwise if the directory already exists `cd build` and run `cmake ..`. This will generate the makefile.  
3. Now to build the project, run `make all`. The generated `libagent.so` will be in the `build` directory.


# Agent Start-Up Commands
These commands are passed to the agent on start up. The format of the commands should be a comma-seperated list of key:value pairs without white-space provided at the end of the path used to load and run the agent. ie) -agentpath:c:\myLibs\foo.dll=**key1:val1,key2:val2**

| Key | Value | Description |
| --- | ---| --- |
| commandFile | Path to file | Provide the agent with a path to a file containing commands (see [Function Commands](#function-commands)) that will be fed to the agent in headless mode |
| logFile | Path to file | Provide the agent with a location to place a log file for agent output. The agent will create a new file if it does not exist. |
| portNo | Port Number | Provide the agent with a port to start the server on. The default port is 9002.  


# Function Commands
These commands are provided by either a commands file (see [Agent Start-Up Commands](#agent-start-up-commands)), or by a live client during runtime. These commands dictate what information the agent collects by either stopping or starting certain capabilities. 

| Functionality | Command | Other Fields | Description |
| --- | --- | --- | --- | 
| monitorEvents | start, stop | delay | Enable/Disable collection of contended monitor events. Delay denotes how many seconds the agent will wait to execute the command. |
| objectAllocEvents | start, stop | delay | Enable/Disable collection of VM object allocation events. Delay denotes how many seconds the agent will wait to execute the command. |
| perf | (time) | delay | Enable perf collection for (time) seconds. Delay denotes how many seconds the agent will wait to execute the command. |

Additionaly, each input should contain a delay field that indicates how long the agent should wait before executing the command.

All commands are provided in JSON format, where multiple commands are provided as a list. A sample command file might look like:
```
{
  "1": {
    "functionality": "monitorEvents",
    "command": "start",
    "delay": 0
  },
  "2": {
    "functionality": "perf",
    "time": 1
  },
  "3": {
    "functionality": "objectAllocEvents",
    "command": "start",
    "delay": 0
  },
  "4": {
    "functionality": "monitorEvents",
    "command": "stop",
    "delay": 4
  },
  "5": {
    "functionality": "objectAllocEvents",
    "command": "stop",
    "delay": 6
  }
}
```