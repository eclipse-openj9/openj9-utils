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

import org.objectweb.asm.tree.analysis.BasicValue;

import org.objectweb.asm.Type;

/**
 * Represents an instance of <code>java/lang/reflect/Method</code>.
 * 
 * The named method is an exact match - eg the reflection object will refer
 * specifically to the named class and method. If the method is invoked on
 * a receiver then usual virtual dispatch or interface dispatch rules will
 * apply and could dispatch to an override of the named method.
 */
public class MethodValue extends BasicValue {
    private String clazz;
    private String name;
    private String desc;
    
    public MethodValue(String clazz, String name, String desc) {
        super(Type.getObjectType("java/lang/reflect/Method"));
        this.clazz = clazz;
        this.name = name;
        this.desc = desc;
    }
    
    public MethodValue(MethodValue v) {
        super(Type.getObjectType("java/lang/reflect/Method"));
        this.clazz = v.clazz;
        this.name = v.name;
        this.desc = v.desc;
    }
    
    public String getClazz() {
        return clazz;
    }
    
    public String getName() {
        return name;
    }
    
    public String getDesc() {
        return desc;
    }
    
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null) {
            return false;
        }
        if (o instanceof MethodValue) {
            MethodValue m = ((MethodValue)o);
            return m.clazz.equals(clazz) && m.name.equals(name) && m.desc.equals(desc);
        }
        return false;
    }
}