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

package org.eclipse.openj9.jmin.util;

import java.util.ArrayList;
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
public class SuperMap extends HashMap<String, ArrayList<String>> {

    private static final long serialVersionUID = -3099743932266613366L;

    public SuperMap() {
        super();
    }

    public SuperMap(int initialCapacity) {
        super(initialCapacity);
    }

    public SuperMap(int initialCapacity, float loadFactor) {
        super(initialCapacity, loadFactor);
    }

    public SuperMap(Map<String, ArrayList<String>> other) {
        super(other);
    }

    public SuperMap(SuperMap other) {
        super(other);
    }
}
