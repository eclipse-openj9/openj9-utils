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

package org.eclipse.openj9.jmin.writer;

import org.eclipse.openj9.jmin.util.Config;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.FieldVisitor;
import org.objectweb.asm.MethodVisitor;

import static org.objectweb.asm.Opcodes.ASM8;

import org.eclipse.openj9.jmin.info.ReferenceInfo;

public class FilteringClassWriterAdapter extends ClassVisitor {
    private String clazz;
    private int mode;
    private ReferenceInfo info;

    public FilteringClassWriterAdapter(int mode, ReferenceInfo info, final ClassVisitor next) {
        super(ASM8, next);
        this.mode = mode;
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
        clazz = name;
        if (cv != null) {
        cv.visit(version, access, name, signature, superName, interfaces);
        }
    }
    @Override
    public MethodVisitor visitMethod(int access, java.lang.String name, java.lang.String descriptor, java.lang.String signature, java.lang.String[] exceptions) {
        if (cv != null
            && (mode == Config.REDUCTION_MODE_CLASS || (info.isClassReferenced(clazz) && info.getClassInfo(clazz).isMethodReferenced(name, descriptor)))) {
            return cv.visitMethod(access, name, descriptor, signature, exceptions);
        }
        return null;
    }
    @Override
    public FieldVisitor visitField(
        final int access,
        final String name,
        final String descriptor,
        final String signature,
        final Object value) {
        if (cv != null
            && (mode == Config.REDUCTION_MODE_CLASS
                || mode == Config.REDUCTION_MODE_METHOD
                || (info.isClassReferenced(clazz) && info.getClassInfo(clazz).isFieldReferenced(name, descriptor)))) {
        return cv.visitField(access, name, descriptor, signature, value);
        }
        return null;
    }
}