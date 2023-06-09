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

package org.eclipse.openj9.jmin.analysis;

import org.objectweb.asm.tree.analysis.BasicValue;

import org.objectweb.asm.Type;

/**
 * Represents an instance of <code>java/lang/reflect/Field</code>.
 *
 * The named field is an exact match - eg the reflection object will refer
 * specifically to the named class and field.
 */
public class FieldValue extends BasicValue {
    private String clazz;
    private String name;

    public FieldValue(String clazz, String name) {
        super(Type.getObjectType("java/lang/reflect/Field"));
        this.clazz = clazz;
        this.name = name;
    }

    public FieldValue(FieldValue v) {
        super(Type.getObjectType("java/lang/reflect/Field"));
        this.clazz = v.clazz;
        this.name = v.name;
    }

    public String getClazz() {
        return clazz;
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
        if (o instanceof FieldValue) {
            FieldValue f = ((FieldValue)o);
            return f.clazz.equals(clazz) && f.name.equals(name);
        }
        return false;
    }
}
