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

import org.eclipse.openj9.jmin.info.ClassInfo;
import org.eclipse.openj9.jmin.info.MethodInfo;
import org.eclipse.openj9.jmin.info.ReferenceInfo;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.eclipse.openj9.jmin.util.WorkList;
import org.objectweb.asm.AnnotationVisitor;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.objectweb.asm.Opcodes.ASM8;

public class OsgiAnnotationProcessor extends AnnotationProcessor {

    class OsgiMethodAnnotationVisitor extends AnnotationVisitor {
        private String mname;

        public OsgiMethodAnnotationVisitor(java.lang.String mname, AnnotationVisitor next) {
            super(ASM8, next);
            this.mname = mname;
        }

        @Override
        public void visit(String name, Object value) {
            /* Refer https://docs.osgi.org/javadoc/osgi.cmpn/7.0.0/org/osgi/service/component/annotations/Reference.html#updated--
             * for rules on finding the name of the updated or unbind method
             */

            if (name.equals("updated") || name.equals("unbind")) {
                if (value.equals("-")) {
                    return;
                }
                String methodName = (String)value;
                if (methodName.equals("")) {
                    Pattern pattern = Pattern.compile("(bind)|(set)|(add)");
                    Matcher matcher = pattern.matcher(mname);
                    if (matcher.matches()) {
                        methodName = matcher.replaceFirst(name);
                    } else {
                        methodName = name + mname;
                    }
                }
                /* For now add all the methods with the candidate name, irrespective of the signature */
                ClassInfo ci = info.getClassInfo(clazz);
                if (ci != null) {
                    for (MethodInfo mi : ci.getMethodsByNameOnly(methodName)) {
                        worklist.processMethod(clazz, methodName, mi.desc());
                    }
                }
            }
        }
    };

    public OsgiAnnotationProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info, ClassVisitor next) {
        super(worklist, context, info, next);
        prefixes = new String[] {
                "Lorg/osgi/service/component/annotations/"
        };
        classAnnotations = new String[] {
                "Component"
        };
        methodAnnotations = new String[] {
                "Activate",
                "Deactivate",
                "Modified",
                "Reference",
        };
    }

    @Override
    public void visit(
            final int version,
            final int access,
            final String name,
            final String signature,
            final String superName,
            final String[] interfaces) {
        if (name.equalsIgnoreCase("org/osgi/enroute/examples/quickstart/dictionaryservice/DictionaryServiceImpl")) {
            System.out.println("Processing OSGI annotation for DictionaryServiceImpl");
        }
        super.visit(version, access, name, signature, superName, interfaces);
    }

    @Override
    public MethodVisitor visitMethod(int maccess, java.lang.String mname, java.lang.String mdesc, java.lang.String msignature, java.lang.String[] mexceptions) {
        return new MethodVisitor(ASM8, cv != null ? cv.visitMethod(maccess, mname, mdesc, msignature, mexceptions) : null) {
            @Override
            public AnnotationVisitor visitAnnotation(java.lang.String desc, boolean visible) {
                if (matchesAnnotation(desc, methodAnnotations)) {
                    worklist.processMethod(clazz, mname, mdesc);
                    if (desc.equals(prefixes + "Reference")) {
                        return new OsgiMethodAnnotationVisitor(mname, mv != null ? mv.visitAnnotation(desc, visible) : null);
                    }
                }
                if (mv != null) {
                    return mv.visitAnnotation(desc, visible);
                }
                return null;
            }
        };
    }
}
