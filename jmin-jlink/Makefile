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
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0

ifneq ($(MAKECMDGOALS),clean)
ifndef JAVA_HOME
$(error JAVA_HOME must be exported to the environment or explicitly set in the make command)
endif # JAVA_HOME
endif # ($(MAKECMDGOALS),clean)

JAR           := $(JAVA_HOME)/bin/jar
JAVAC         := $(JAVA_HOME)/bin/javac
JLINK         := $(JAVA_HOME)/bin/jlink

OUT           := build

# Compile `hellomodule`, plugin, agent and create agent JAR
all : \
	$(OUT)/hellomodule.marker \
	$(OUT)/jminjlinkagent.jar \
	$(OUT)/plugin.marker

# Delete the build output directory
clean :
	rm -rf $(OUT)

# Compile the hellomodule module
$(OUT)/hellomodule.marker : \
		src/module-info.java \
		$(wildcard src/hellomodule/*.java)
	@mkdir -p $(OUT)/modules/hellomodule
	$(JAVAC) \
		-d $(OUT)/modules/hellomodule \
		src/module-info.java \
		src/hellomodule/*.java
	touch $@

# Compile the custom jlink plugin
$(OUT)/plugin.marker : \
		plugin/jmin.jlink/module-info.java \
		$(wildcard plugin/jmin.jlink/jmin/jlink/plugin/*.java)
	@mkdir -p $(OUT)/plugin
	$(JAVAC) \
		-d $(OUT)/plugin \
		--add-exports jdk.jlink/jdk.tools.jlink.internal=jmin.jlink \
		--add-exports jdk.jlink/jdk.tools.jlink.plugin=jmin.jlink \
		plugin/jmin.jlink/module-info.java \
		plugin/jmin.jlink/jmin/jlink/plugin/*.java
	touch $@

# Compile the java agent and create agent JAR
$(OUT)/jminjlinkagent.jar : \
		agent/plugin.agent/MANIFEST.MF \
		agent/plugin.agent/module-info.java \
		$(wildcard agent/plugin.agent/jmin/jlink/agent/*.java)
	@mkdir -p $(OUT)/agent
	$(JAVAC) \
		-d $(OUT)/agent \
		--add-exports jdk.jlink/jdk.tools.jlink.internal=plugin.agent \
		--add-exports jdk.jlink/jdk.tools.jlink.plugin=plugin.agent \
		agent/plugin.agent/module-info.java \
		agent/plugin.agent/jmin/jlink/agent/*.java
	$(JAR) \
		--create \
		--file=$(OUT)/jminjlinkagent.jar \
		--manifest=agent/plugin.agent/MANIFEST.MF \
		-C $(OUT)/agent module-info.class \
		-C $(OUT)/agent jmin

# Run the custom jlink plugin with hellomodule
test : all
	$(JLINK) \
		-J-javaagent:$(OUT)/jminjlinkagent.jar \
		-J--module-path=:$(OUT)/plugin \
		-J--add-modules=jmin.jlink \
		--minimize-classes \
		--module-path=$(OUT)/modules \
		--add-modules hellomodule \
		--output $(OUT)/image \
			2>&1 | tee minimizeclasses.out
