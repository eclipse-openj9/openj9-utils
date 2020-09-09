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

import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

import org.objectweb.asm.Type;
import org.objectweb.asm.tree.AbstractInsnNode;
import org.objectweb.asm.tree.MethodInsnNode;
import org.objectweb.asm.tree.analysis.AnalyzerException;
import org.objectweb.asm.tree.analysis.Frame;
import org.objectweb.asm.tree.analysis.Interpreter;
import org.objectweb.asm.tree.analysis.BasicValue;

import jdk.internal.org.objectweb.asm.Opcodes;

public class InterospectiveFrame extends Frame<BasicValue> {
    public InterospectiveFrame(final int nlocals, final int nstack) {
        super(nlocals, nstack);
    }
    public InterospectiveFrame(final Frame<? extends BasicValue> src) {
        super(src);
    }
    public void replaceValue(BasicValue orig, BasicValue next) {
        Stack<BasicValue> mirror = new Stack<BasicValue>();
        for (int i = 0, e = getStackSize(); i < e; ++i) {
            BasicValue v = pop();
            mirror.push(v == orig ? next : v);
        }
        for (int i = 0, e = mirror.size(); i < e; ++i) {
            push(mirror.pop());
        }
        for (int i = 0, e = getLocals(); i < e; ++i) {
            BasicValue local = getLocal(i);
            if (local == orig) {
                setLocal(i, next);
            }
        }
    }
    @Override
    public void execute(final AbstractInsnNode insn, final Interpreter<BasicValue> interpreter) throws AnalyzerException {
        switch (insn.getOpcode()) {
            case Opcodes.INVOKEVIRTUAL:
            case Opcodes.INVOKESPECIAL:
            case Opcodes.INVOKESTATIC:
            case Opcodes.INVOKEINTERFACE: {
                List<BasicValue> values = new ArrayList<BasicValue>();
                String desc = ((MethodInsnNode) insn).desc;
                for (int i = Type.getArgumentTypes(desc).length; i > 0; --i) {
                    values.add(0, pop());
                }
                if (insn.getOpcode() != Opcodes.INVOKESTATIC) {
                    values.add(0, pop());
                }
                if (Type.getReturnType(desc) == Type.VOID_TYPE) {
                    if (interpreter instanceof ReflectionInterpreter) {
                        ((ReflectionInterpreter)interpreter).naryOperation(insn, values, this);
                    } else {
                        interpreter.naryOperation(insn, values);
                    }
                } else {
                    if (interpreter instanceof ReflectionInterpreter) {
                        push(((ReflectionInterpreter)interpreter).naryOperation(insn, values, this));
                    } else {
                        push(interpreter.naryOperation(insn, values));
                    }
                }
                return;
            }
        }
        super.execute(insn, interpreter);
    }
}
