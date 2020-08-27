#!/bin/bash
#
# Copyright (c) 2020, 2020 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
#
if [ ! -d "./lib" ]; then
  mkdir lib
fi
if [ ! -f "./lib/asm-8.0.1.jar" ]; then
  curl https://repository.ow2.org/nexus/content/repositories/releases/org/ow2/asm/asm/8.0.1/asm-8.0.1.jar --output ./lib/asm-8.0.1.jar
fi
if [ ! -f "./lib/asm-analysis-8.0.1.jar" ]; then
  curl https://repository.ow2.org/nexus/content/repositories/releases/org/ow2/asm/asm-analysis/8.0.1/asm-analysis-8.0.1.jar --output ./lib/asm-analysis-8.0.1.jar
fi
if [ ! -f "./lib/asm-tree-8.0.1.jar" ]; then
  curl https://repository.ow2.org/nexus/content/repositories/releases/org/ow2/asm/asm-tree/8.0.1/asm-tree-8.0.1.jar --output ./lib/asm-tree-8.0.1.jar
fi
if [ ! -f "./lib/asm-util-8.0.1.jar" ]; then
  curl https://repository.ow2.org/nexus/content/repositories/releases/org/ow2/asm/asm-util/8.0.1/asm-util-8.0.1.jar --output ./lib/asm-util-8.0.1.jar
fi
if [ ! -d "./bin" ]; then
  mkdir bin
fi
cd src
$JAVA_HOME/bin/javac -d ../bin -cp ../lib/asm-8.0.1.jar:../lib/asm-analysis-8.0.1.jar:../lib/asm-tree-8.0.1.jar:../lib/asm-util-8.0.1.jar:. org/eclipse/openj9/JMin.java
cd ..
if [ -f "./jarmin.jar" ]; then
  rm -f ./jarmin.jar
fi
echo "Main-Class: org.eclipse.openj9.JMin" > ./bin/manifest
echo "Class-Path: lib/asm-8.0.1.jar lib/asm-analysis-8.0.1.jar lib/asm-tree-8.0.1.jar lib/asm-util-8.0.1.jar" >> ./bin/manifest
$JAVA_HOME/bin/jar -cfm jarmin.jar ./bin/manifest -C ./bin org
