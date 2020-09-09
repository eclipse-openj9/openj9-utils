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

package org.eclipse.openj9.jmin.analysis;

import org.objectweb.asm.Type;
import org.objectweb.asm.tree.analysis.BasicValue;

/**
 * Represents an instance of <code>java/lang/Class</code>
 *
 * The named class is an extact match (eg you know the class is spefically the
 * <code>java/lang/Class</code> for the named type and not a subclass)
 */
public class ClassValue extends BasicValue {
    private String name;

    public ClassValue(String name) {
        super(Type.getObjectType("java/lang/Class"));
        this.name = name;
    }

    public ClassValue(ClassValue v) {
        super(Type.getObjectType("java/lang/Class"));
        this.name = new String(v.getName());
    }

    public String getName() {
        return name;
    }

    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null) {
            return false;
        }
        if (o instanceof ClassValue) {
            String oname = ((ClassValue)o).name;
            return (name == null && oname == null) || (oname != null && oname.equals(name));
        }
        return false;
    }
}
