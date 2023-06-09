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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

package org.eclipse.openj9.jmin.util;

import org.eclipse.openj9.jmin.info.ClassSource;

import java.util.HashMap;
import java.util.Map;

/**
 * A simple Map wrapper used to track the superclass chain of a given class.
 *
 * The {@code ArrayList<String>} is ordered from immediate parent to most distant.
 *
 * @version 1.0
 * @since 1.0
 * @see java.util.HashMap
 */
public class JarMap extends HashMap<String, ClassSource> {

    private static final long serialVersionUID = -3099743932266613366L;

    public JarMap() {
        super();
    }

    public JarMap(int initialCapacity) {
        super(initialCapacity);
    }

    public JarMap(int initialCapacity, float loadFactor) {
        super(initialCapacity, loadFactor);
    }

    public JarMap(Map<String, ClassSource> other) {
        super(other);
    }

    public JarMap(JarMap other) {
        super(other);
    }
}
