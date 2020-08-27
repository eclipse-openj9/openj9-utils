<!--
Copyright (c) 2020, 2020 IBM Corp. and others

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] http://openjdk.java.net/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
-->

# jmin-jlink

Includes:
- **hellomodule.HelloWorld**, a simple app that contains classes, methods and fields - some of which should be removed during minimization;
- **MinimimizeClassesPlugin**, a jlink plugin that minimizes an image using jarmin/jmin (jarmin/jmin-related code is commented out)
- **MinimizeClassesPluginAgent**, a java agent that allows a custom implementation of a plugin to be used with jlink

---

## How to Run
This command runs jlink with the custom plugin (jlink option `--minimize-classes`), enabled by the java agent, 
on a simple app contained in `hellomodule`. The resulting image is generated in the `build` directory, and output 
from the command is piped to `minimizeclasses.out`.

As this code involves Java modularity, Java 11 or above is required.

From the `jmin-jlink` directory, run the following:

#### Compile the hellomodule classes
```
javac -d out --module-path out src/module-info.java src/hellomodule/*.java
```

#### Compile the custom jlink plugin
```
javac --add-exports jdk.jlink/jdk.tools.jlink.plugin=jmin.jlink --add-exports jdk.jlink/jdk.tools.jlink.internal=jmin.jlink plugin/jmin.jlink/module-info.java plugin/jmin.jlink/jmin/jlink/plugin/*.java
```

#### Compile the java agent
```
javac --add-exports jdk.jlink/jdk.tools.jlink.internal=plugin.agent --add-exports jdk.jlink/jdk.tools.jlink.plugin=plugin.agent agent/plugin.agent/module-info.java agent/plugin.agent/jmin/jlink/agent/*.java
cd agent/plugin.agent
jar cfm jminjlinkagent.jar MANIFEST.mf jmin/jlink/agent/*.class *.class
cd ../..
```

#### Run the custom jlink plugin with hellomodule
You may need to use the absolute path to `jlink` here. The `jlink` tool found in the `bin` directory of the JDK.
```
jlink -J-javaagent:agent/plugin.agent/jminjlinkagent.jar -J--module-path=:plugin -J--add-modules=jmin.jlink --minimize-classes --module-path=out --add-modules hellomodule --output build > minimizeclasses.out
```

#### View the output in minimizeclasses.out
Here's a sample of what the first few lines of output looks like:
```
previsiting!!

java.base
/java.base/module-info.class
CLASS_OR_RESOURCE

java.logging
/java.logging/module-info.class
CLASS_OR_RESOURCE

hellomodule
/hellomodule/module-info.class
CLASS_OR_RESOURCE
```

---

## Build and run with your changes

### Simple Sample App
#### Build the hellomodule sample app

**HelloWorld.java**: contains main method and classes/methods/fields, some of which should be removed via minimization\
**ReferencedClass.java**: a class that is referenced in HelloWorld.java, so it should be included in the image\
**UnusedClass.java**: a class that is not used in the sample app, so it should be removed from the image

```
javac -d out --module-path out src/module-info.java src/hellomodule/*.java
```

#### Run the hellomodule sample app
```
java --module-path out --module hellomodule/hellomodule.HelloWorld
```

EXAMPLE OUTPUT:
```
Jul. 27, 2020 6:31:15 P.M. hellomodule.HelloWorld main
INFO: Hello World!
ReferencedClass.aMethod()
```

#### Create a hellomodule jar (to use with jarmin)
```
cd out
jar cf hellomodule.jar hellomodule module-info.class
```

### Plugin
MinimizeClassesPlugin custom plugin for jlink

#### Build plugin
From the top-level `jmin-jlink` directory:

```
javac --add-exports jdk.jlink/jdk.tools.jlink.plugin=jmin.jlink --add-exports jdk.jlink/jdk.tools.jlink.internal=jmin.jlink plugin/jmin.jlink/module-info.java plugin/jmin.jlink/jmin/jlink/plugin/*.java
```

### Agent
MinimizeClassesPluginAgent, Java Instrumentation Agent for the custom jlink plugin

#### Build agent and create jar
From the top-level `jmin-jlink` directory:

```
javac --add-exports jdk.jlink/jdk.tools.jlink.internal=plugin.agent --add-exports jdk.jlink/jdk.tools.jlink.plugin=plugin.agent agent/plugin.agent/module-info.java agent/plugin.agent/jmin/jlink/agent/*.java
cd agent/plugin.agent
jar cfm jminjlinkagent.jar MANIFEST.mf jmin/jlink/agent/*.class *.class
```

### Run plugin with jlink

Go to the top-level `jmin-jlink` directory.
The resulting image will be generated in the `build` directory and output generated by the command/plugin will be in `minimizeclasses.out`.

```
jlink -J-javaagent:agent/plugin.agent/jminjlinkagent.jar -J--module-path=:plugin -J--add-modules=jmin.jlink --minimize-classes --module-path=out --add-modules hellomodule --output build > minimizeclasses.out
```

---

### Other Useful Commands

From the top-level `jmin-jlink` directory:\
`jdeps <JDEPS_OPTIONS> out/hellomodule.jar`
- `<JDEPS_OPTIONS>`: --list-deps, --print-module-deps

Note that `jdeps` is a static analysis tool, so it doesn't identify modules that are dynamically required, such as those required in reflection or service providers.
