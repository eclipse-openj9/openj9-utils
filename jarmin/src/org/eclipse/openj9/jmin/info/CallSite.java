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

package org.eclipse.openj9.jmin.info;

import org.objectweb.asm.tree.analysis.BasicValue;

public class CallSite {
    public final MethodInfo caller;
    public final String clazz;
    public final String name;
    public final String desc;
    public final CallKind kind;
    public final int instructionIndex;
    private ClassSource classSource;
    private BasicValue[] argValueList;
    public CallSite(MethodInfo caller, String clazz, String name, String desc, CallKind kind, int instructionIndex, ClassSource classSource) {
        //if (clazz.equals("com/arjuna/ats/internal/jta/Implementationsx"))
        //throw new RuntimeException();
        this.caller = caller;
        this.clazz = new String(clazz.toCharArray());
        this.name = new String(name.toCharArray());
        this.desc = new String(desc.toCharArray());
        this.kind = kind;
        this.instructionIndex = instructionIndex;
        this.classSource = classSource;
        this.argValueList = null;
    }

    public int getInstructionIndex() { return instructionIndex; }

    public MethodInfo getCaller() {
        return caller;
    }

    public void setArgValueList(BasicValue[] argValueList) {
        this.argValueList = argValueList;
    }

    public BasicValue getArgValue(int argIndex) {
        return argValueList[argIndex];
    }

    public BasicValue[] getAllArgValues() {
        return argValueList;
    }

    @Override
    public String toString() {
        return "CallSite " + kind.toString() + ": " + clazz + "." + name + desc;
    }

    public ClassSource getClassSource() {
        return classSource;
    }
}
