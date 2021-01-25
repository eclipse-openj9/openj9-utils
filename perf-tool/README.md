# Environment Setup
1. Make sure you have environment variable JAVA_HOME set to point to a Java SDK directory. Example (in bash):
```
JAVA_HOME=/usr/lib/jvm/adoptopenjdk-8-openj9-amd64/
```

# Build Instructions
1. Make sure you have CMake installed & updated. To do so on ubuntu, run `sudo apt-get install cmake`.  
2. Create build directory `mkdir build`, otherwise if the directory already exists `cd build` and run `cmake ..`. This will generate the makefile.  
3. Now to build the project, run `make all`. The generated `libagent.so` will be in the `build` directory.

# Perf Setup
1. Prior to running commands to collect perf data, ensure perf is installed on your computer.
### Ubuntu
```
sudo apt install linux-tools-common
```
### Debian
```
sudo apt install linux-perf
```
2. Running perf as a non-root user may cause conflicts. To overcome these conflicts, you will need to change some kernel parameters:
```
sudo sh -c " echo 0 > /proc/sys/kernel/kptr_restrict"
```
```
sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'
```

# Agent Start-Up Commands
These commands are passed to the agent on start up. The format of the commands should be a comma-seperated list of key:value pairs without white-space provided at the end of the path used to load and run the agent. ie) -agentpath:c:\myLibs\foo.dll=**key1:val1,key2:val2**

| Key | Value | Description |
| --- | ---| --- |
| commandFile | Path to file | Provide the agent with a path to a file containing commands (see [Function Commands](#function-commands)) that will be fed to the agent in headless mode |
| logFile | Path to file | Provide the agent with a location to place a log file for agent output. The agent will create a new file if it does not exist. |
| portNo | Port Number | Provide the agent with a port to start the server on. The default port is 9002.  


# Function Commands
These commands are provided by either a commands file (see [Agent Start-Up Commands](#agent-start-up-commands)), or by a live client during runtime. These commands dictate what information the agent collects by either stopping or starting certain capabilities.

| Command | Associated Events | Expected Value | Description |
| --- | --- | --- | ---- |
| start | monitorEvents, objectAllocEvents, methodEntryEvents, exceptionEvents | Event Name | Start recording an event |
| stop | monitorEvents, objectAllocEvents, methodEntryEvents, exceptionEvents | Event Name | Stop recording an event |
| sampleRate | objectAllocEvents, methodEntryEvents*, exceptionEvents | Event Name | Set a sampling rate `n` for retrieving backtrace (set to 0 for none) *methodEntryEvents required to have sampleRate > 0 |
| delay | All Functionalities | Integer | Time to wait before running the command after it is received (in seconds) |
| time | perf | Integer | Time to run the command for |

All commands are provided in JSON format, where multiple commands are provided as a list. A sample command file might look like:
```
[
  {
    "functionality": "monitorEvents",
    "command": "start",
    "delay": 0
  },
  {
    "functionality": "perf",
    "time": "1"
  },
  {
    "functionality": "exceptionEvents",
    "command": "start",
    "sampleRate": 5,
    "delay": 0
  },
  {
    "functionality": "objectAllocEvents",
    "command": "start",
    "sampleRate": 2,
    "delay": 0
  },
  {
    "functionality": "methodEntryEvents",
    "command": "start",
    "sampleRate": 2000,
    "delay": 0
  },
  {
    "functionality": "methodEntryEvents",
    "command": "stop",
    "delay": 4
  },
  {
    "functionality": "monitorEvents",
    "command": "stop",
    "delay": 4
  },
  {
    "functionality": "objectAllocEvents",
    "command": "stop",
    "delay": 6
  },
  {
    "functionality": "exceptionEvents",
    "command": "stop",
    "delay": 6
  }
]

```
