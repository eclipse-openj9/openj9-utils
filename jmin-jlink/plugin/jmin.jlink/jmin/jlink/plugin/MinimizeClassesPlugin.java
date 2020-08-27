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

package jmin.jlink.plugin;

import java.io.UnsupportedEncodingException;
import jdk.tools.jlink.plugin.Plugin;
import jdk.tools.jlink.plugin.ResourcePool;
import jdk.tools.jlink.plugin.ResourcePoolBuilder;
import jdk.tools.jlink.plugin.ResourcePoolEntry;
import jdk.tools.jlink.internal.ResourcePrevisitor;
import jdk.tools.jlink.internal.StringTable;

/**
 * A jlink plugin that minimizes image classes by removing unnecessary
 * classes, methods and fields from the image.
 */
public class MinimizeClassesPlugin implements Plugin, ResourcePrevisitor {
	private static final String NAME = "minimize-classes";

	@Override
	public String getName() {
		return NAME;
	}

	@Override
	public String getDescription() {
		return "Minimize classes in image by removing unnecessary classes, methods, fields.";
	}

	@Override
	public Category getType() {
		return Category.COMPRESSOR;
	}

	@Override
	public void previsit(ResourcePool resourcePool, StringTable stringTable) {
		// construct JMin & fill in HierarchyContext
		System.out.println("previsiting!!");
	}

	/**
	 * This method makes a copy of the ResourcePool.
	 * @param resourcePool Pool of resources
	 * @param resourcePoolBuilder Builder to create a copy of the pool of resources.
	 * @return ResourcePool Built copy of the pool of resources.
	 */
	public ResourcePool transform(ResourcePool resourcePool, ResourcePoolBuilder resourcePoolBuilder) {
		// populate worklist
		// process entries
		// minimize classes
		// copy each minimized class to the output resourcepool
		resourcePool.transformAndCopy(
			resourcePoolEntry -> {
				/* Transform each resourcePoolEntry in some way */

				System.out.println();

				System.out.println(resourcePoolEntry.moduleName());
				System.out.println(resourcePoolEntry.path());

				/**
				 * Resource Pool Entry Types
				 *
				 * CLASS_OR_RESOURCE: A java class or resource file.
				 * CONFIG: A configuration file.
				 * HEADER_FILE: A header file.
				 * LEGAL_NOTICE: A legal notice.
				 * MAN_PAGE: A man page.
				 * NATIVE_CMD: A native executable launcher.
				 * NATIVE_LIB: A native library.
				 * TOP: A top-level file in the jdk run-time image directory.
				 */
				System.out.println(resourcePoolEntry.type());

				return resourcePoolEntry;
			},
			/* Each transformed entry is copied to the resourcePoolBuilder */
			resourcePoolBuilder
		);

		return resourcePoolBuilder.build();
	}
}
