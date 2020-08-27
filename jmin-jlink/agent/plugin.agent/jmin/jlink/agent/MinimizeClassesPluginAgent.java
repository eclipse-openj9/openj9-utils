/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

package jmin.jlink.agent;

import java.lang.instrument.Instrumentation;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Java agent that registers the MinimizeClassesPlugin as a jlink plugin service.
 */
public class MinimizeClassesPluginAgent {
	public static void premain(String arguments, Instrumentation instrumentation) {
		Module jlinkModule = ModuleLayer.boot().findModule("jdk.jlink").get();
		Module minimizeClassesModule = ModuleLayer.boot().findModule("jmin.jlink").get();

		// Create hashmap to hold extra exports for minimizeClassesModule
		Map<String, Set<Module>> extraModuleExports = new HashMap<>(2);
		extraModuleExports.put("jdk.tools.jlink.plugin", Collections.singleton(minimizeClassesModule));
		extraModuleExports.put("jdk.tools.jlink.internal", Collections.singleton(minimizeClassesModule));

		// Redefine the jlinkModule to include an extra export for minimizeClassesModule
		instrumentation.redefineModule(
			jlinkModule,
			Collections.emptySet(),
			extraModuleExports,
			Collections.emptyMap(),
			Collections.emptySet(),
			Collections.emptyMap()
		);

		Class<?> jlinkPluginClass = null;
		Class<?> minimizeClassesPluginClass = null;

		try {
			jlinkPluginClass = jlinkModule.getClassLoader().loadClass("jdk.tools.jlink.plugin.Plugin");
			minimizeClassesPluginClass = minimizeClassesModule.getClassLoader().loadClass("jmin.jlink.plugin.MinimizeClassesPlugin");
		} catch (ClassNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		// Create hashmap to hold extra provides for MinimizeClassesPlugin class
		Map<Class<?>, List<Class<?>>> extraModuleProvides = new HashMap<>(1);
		extraModuleProvides.put(jlinkPluginClass, Collections.singletonList(minimizeClassesPluginClass));

		// Redefine the jlinkModule to provide the MinimizeClassesPlugin as a plugin service
		instrumentation.redefineModule(
			minimizeClassesModule,
			Collections.emptySet(),
			Collections.emptyMap(),
			Collections.emptyMap(),
			Collections.emptySet(),
			extraModuleProvides
		);
	}
}
