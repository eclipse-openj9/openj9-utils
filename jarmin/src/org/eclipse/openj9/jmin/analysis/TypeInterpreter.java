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

import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.Type;
import org.objectweb.asm.tree.AbstractInsnNode;
import org.objectweb.asm.tree.TypeInsnNode;
import org.objectweb.asm.tree.analysis.AnalyzerException;
import org.objectweb.asm.tree.analysis.BasicInterpreter;
import org.objectweb.asm.tree.analysis.BasicValue;

public class TypeInterpreter extends BasicInterpreter {
    private HierarchyContext context;
    public TypeInterpreter(HierarchyContext context) {
        super(ASM8);
        this.context = context;
    }

    @Override
    public BasicValue newOperation(final AbstractInsnNode insn) throws AnalyzerException {
        if (insn.getOpcode() == Opcodes.NEW) {
            return new FixedTypeValue(Type.getObjectType(((TypeInsnNode) insn).desc));
        }
        return super.newOperation(insn);
    }

    @Override
    public BasicValue newValue(final Type type) {
        if (type != null
            && type.getSort() == Type.OBJECT
            && type.getDescriptor() != null) {
            return new TypeValue(type);
        }
        return super.newValue(type);
    }

    @Override
    public BasicValue merge(BasicValue v1, BasicValue v2) {
        if (v1 instanceof TypeValue && v2 instanceof TypeValue) {
            String desc1 = v1.getType().getDescriptor(), desc2 = v2.getType().getDescriptor();

            if (desc1.equals(desc2) || context.getSuperClasses(desc2).contains(desc1)) {
                return v1;
            } else if (context.getSuperClasses(desc1).contains(desc2)) {
                return v2;
            }
        } else if (v1 instanceof FixedTypeValue && v2 instanceof FixedTypeValue) {
            String desc1 = v1.getType().getDescriptor(), desc2 = v2.getType().getDescriptor();
            if (desc1.equals(desc2)){
                return v1;
            } else if (context.getSuperClasses(desc2).contains(desc1)) {
                return new TypeValue(v1.getType());
            } else if (context.getSuperClasses(desc1).contains(desc2)) {
                return new TypeValue(v2.getType());
            }
        } else if (v1 instanceof FixedTypeValue && v2 instanceof TypeValue) {
            String desc1 = v1.getType().getDescriptor(), desc2 = v2.getType().getDescriptor();
            if (desc1.equals(desc2)) {
                return v2;
            } else if (context.getSuperClasses(desc1).contains(desc2)) {
                return v2;
            } else if (context.getSuperClasses(desc2).contains(desc1)) {
                return new TypeValue(v1.getType());
            }
        } else if (v1 instanceof TypeValue && v2 instanceof FixedTypeValue) {
            String desc1 = v1.getType().getDescriptor(), desc2 = v2.getType().getDescriptor();
            if (desc1.equals(desc2)) {
                return v1;
            } else if (context.getSuperClasses(desc1).contains(desc2)) {
                return new TypeValue(v2.getType());
            } else if (context.getSuperClasses(desc2).contains(desc1)) {
                return v1;
            }
        }
        return super.merge(degradeValue(v1), degradeValue(v2));
    }

    private BasicValue degradeValue(BasicValue v) {
        if (v instanceof TypeValue || v instanceof FixedTypeValue) {
            return BasicValue.REFERENCE_VALUE;
        }
        return v;
    }
}