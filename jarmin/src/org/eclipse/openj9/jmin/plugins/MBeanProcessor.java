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

package org.eclipse.openj9.jmin.plugins;

import org.eclipse.openj9.jmin.info.MethodInfo;
import org.eclipse.openj9.jmin.info.ReferenceInfo;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.eclipse.openj9.jmin.util.WorkList;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;

import static org.objectweb.asm.Opcodes.ASM8;

public class MBeanProcessor extends ClassVisitor {
    private static final String MBEAN_SUFFIX = "MBean";
    private static final String MXBEAN_SUFFIX = "MXBean";
    private static final String[] suffixes = new String[] { MBEAN_SUFFIX, MXBEAN_SUFFIX };
    private String clazz;
    private WorkList worklist;
    private HierarchyContext context;
    private ReferenceInfo info;
    private boolean trace;
    private boolean matched;
    public MBeanProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info, ClassVisitor next) {
        super(ASM8, next);
        this.worklist = worklist;
        this.context = context;
        this.info = info;
    }
    @Override
    public void visit(
        final int version,
        final int access,
        final String name,
        final String signature,
        final String superName,
        final String[] interfaces) {
        this.clazz = name;
        this.trace = false;
        this.matched = false;
        for (String s : suffixes) {
            if (name.endsWith(s)) {
                matched = true;
                worklist.forceInstantiateClass(name);
                for (String i : context.getInterfaceImplementors(name)) {
                    worklist.forceInstantiateClass(i);
                }
                break;
            }
        }
        if (cv != null) {
            cv.visit(version, access, name, signature, superName, interfaces);
        }
        
    }

    @Override
    public MethodVisitor visitMethod(
        final int access,
        final String name,
        final String descriptor,
        final String signature,
        final String[] exceptions) {
        if (matched) {
            worklist.processMethod(clazz, name, descriptor);
        }
        if (cv != null) {
            return cv.visitMethod(access, name, descriptor, signature, exceptions);
        }
        return null;
    }
    
    @Override
    public void visitEnd() {
        if (matched && trace) {
            System.out.println("@ " + this.getClass().getName() + " matched " + clazz);
        }
        if (cv != null) {
            cv.visitEnd();
        }
    }
}