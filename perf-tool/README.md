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

| Command | Associated Events | Expected Value | Description |
| --- | --- | --- | ---- |
| start | monitorEvents, objectAllocEvents, methodEntryEvents | Event Name | Start recording an event |
| stop | monitorEvents, objectAllocEvents, methodEntryEvents | Event Name | Stop recording an event |
| sampleRate | objectAllocEvents, methodEntryEvents | Event Name | Set a sampling rate `n` for retrieving backtrace (set to 0 for none) |
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
  }
]

```
