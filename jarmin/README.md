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

# Jar Minimizer
The JAR minimizer is a standalone tool for statically processing a set of JAR files to produce copies excluding components not
specifically required for an application. The tool uses the ASM bytecode library and handles reflection and other operations using an
extension of the ASM absract interpreter intraprocedurally.

## Building
```
./build.cmd
```
You need to set JAVA\_HOME to point to a Java 8 JDK before running

## Running
```
$JAVA_HOME/bin/java -jar jarmin.jar <classpath to search> <main class> <main method name> <main method signature>
```
The tool had three system properties that control its operation:

Set org.eclipse.openj9.jmin.reduction\_mode to control how minimization is performed:
- class (remove only unused classes) [default]
- method (remove unused classes and methods)
- field (remove unused classes, methods and fields)

Set org.eclipse.openj9.jmin.inclusion\_mode to control how classes are included in the analysis:
- reference (overriden / implemented methods are scanned when the class is first referenced)
- instantiation (overridden / implemented methods are scanned when the class is first instantiated) [default]

Set org.eclipse.openj9.jmin.trace to control output verbosity:
- true (verbose output will be printed to stdout)
- false (only impotant diagnostic output will be printed to stdout) [default]

Set org.eclipse.openj9.jmin.method\_summary.enable to enable method summary to locate classes used via reflection:
- true [default]
- false

Set org.eclipse.openj9.jmin.type\_refinement.enable to enable type analysis of callsites to reduce the number of included classes:
- true [default]
- false

At the present time the `method` and `field` modes are experimental and do not support fields only initialized but not read after
initialization so results will vary.
