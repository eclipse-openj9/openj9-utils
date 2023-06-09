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

package org.eclipse.openj9.jmin.util;

import java.util.LinkedList;
import java.util.List;

import org.eclipse.openj9.JMin;
import org.eclipse.openj9.jmin.WorkItem;
import org.eclipse.openj9.jmin.info.ClassInfo;
import org.eclipse.openj9.jmin.info.FieldInfo;
import org.eclipse.openj9.jmin.info.MethodInfo;
import org.eclipse.openj9.jmin.info.ReferenceInfo;

public class WorkList {
    private HierarchyContext context;
    private ReferenceInfo info;
    private LinkedList<WorkItem> worklist;

    public WorkList(ReferenceInfo info, HierarchyContext context) {
        this.worklist = new LinkedList<WorkItem>();
        this.context = context;
        this.info = info;
    }

    public boolean hasNext() {
        return worklist.size() > 0;
    }

    public WorkItem next() {
        return worklist.removeFirst();
    }

    private void processMethods(String c) {
        ClassInfo cinfo = info.getClassInfo(c);

        LinkedList<String> classesToCheck = new LinkedList<String>();
        classesToCheck.add(c);
        classesToCheck.addAll(context.getSuperClasses(c));

        while (!classesToCheck.isEmpty()) {
            String check = classesToCheck.pop();
            String[] interfaces = context.getClassInterfaces(check);
            for (int i = 0; i < interfaces.length; ++i) {
                classesToCheck.add(interfaces[i]);
            }
            if (info.getClassInfo(check) != null) {
                for (MethodInfo mi : info.getClassInfo(check).getReferencedMethods()) {
                    if (cinfo.hasMethod(mi.name(), mi.desc())
                        && !cinfo.isMethodReferenced(mi.name(), mi.desc())) {
                        cinfo.markMethodReferenced(mi.name(), mi.desc());
                        worklist.add(new WorkItem(c, mi.name(), mi.desc()));
                    }
                }
            }
        }

        if (Config.reductionMode != Config.REDUCTION_MODE_FIELD) {
            for (FieldInfo field : cinfo.getFields()) {
                if (!field.referenced()) {
                    field.setReferenced();
                    String desc = field.desc();
                    if (desc.charAt(0) == 'L' && desc.charAt(desc.length() - 1) == ';') {
                        String fclazz = desc.substring(1, desc.length() - 1);
                        processClass(fclazz);
                    }
                }
            }
        }
    }

    public void forceInstantiateClass(String clazz) {
        instantiateClass(clazz);
        ClassInfo cinfo = info.getClassInfo(clazz);
        if (cinfo != null) {
            /* Mark all constructors as referenced and add them to worklist */
            for (MethodInfo mi : cinfo.getMethodsByNameOnly("<init>")) {
                cinfo.markMethodReferenced(mi.name(), mi.desc());
                worklist.add(new WorkItem(clazz, mi.name(), mi.desc()));
            }
        }
    }

    public void instantiateClass(String clazz) {
        processClass(clazz);
        if (!info.isClassInstantiated(clazz)) {
            ClassInfo cinfo = info.getClassInfo(clazz);
            if (cinfo == null) {
                handleClass(clazz);
            }
            cinfo = info.getClassInfo(clazz);
            cinfo.setInstantiated();

            List<String> superClazzes = context.getSuperClasses(clazz);
            if (superClazzes != null && superClazzes.size() > 0) {
                instantiateClass(superClazzes.get(0));
            }

            if (context.getServiceProviders(clazz) != null) {
                for (String impl : context.getServiceProviders(clazz)) {
                    instantiateClass(impl);
                }
            } else if (clazz.equals(JMin.ALL_SVC_IMPLEMENTAIONS)) {
                for (String svc : context.getServiceInterfaces()) {
                    for (String impl : context.getServiceProviders(svc)) {
                        instantiateClass(impl);
                    }
                }
            }

            if (Config.inclusionMode == Config.INCLUSION_MODE_INSTANTIATE) {
                processMethods(clazz);
            }
        }
    }

    private void handleClass(String c) {
        if (!info.isClassReferenced(c)) {
            info.addClass(c).setReferenced();
            if (info.getClassInfo(c).hasMethod("<clinit>", "()V")
                && !info.getClassInfo(c).isMethodReferenced("<clinit>", "()V")) {
                info.getClassInfo(c).markMethodReferenced("<clinit>", "()V");
                worklist.add(new WorkItem(c, "<clinit>", "()V"));
            }

            LinkedList<String> classesToCheck = new LinkedList<String>();
            classesToCheck.add(c);
            classesToCheck.addAll(context.getSuperClasses(c));

            while (!classesToCheck.isEmpty()) {
                String check = classesToCheck.pop();
                String[] interfaces = context.getClassInterfaces(check);
                for (int i = 0; i < interfaces.length; ++i) {
                    classesToCheck.add(interfaces[i]);
                }
                processClass(check);
                ClassInfo cinfo = info.getClassInfo(check);
                for (String annot : cinfo.getAnnotations()) {
                    processClass(annot);
                }
            }
        }
    }

    public void processClass(String clazz) {
        /*if (clazz.indexOf('.') != -1) {
            clazz = clazz.replace('.', '/');
            throw new RuntimeException("bad class name" + clazz);
        }*/
        if (!info.isClassReferenced(clazz)) {
            handleClass(clazz);
            for (String i : context.getClassInterfaces(clazz)) {
                processClass(i);
            }
            for (String c : context.getSuperClasses(clazz)) {
                processClass(c);
                for (String i : context.getClassInterfaces(c)) {
                    processClass(i);
                }
            }

            if (Config.inclusionMode == Config.INCLUSION_MODE_REFERENCE) {
                processMethods(clazz);
            }
        }
    }

    public void processField(String clazz, String name, String desc) {
        processClass(clazz);
        String foundClazz = info.markFieldReference(context, clazz, name, desc);
        if (foundClazz == null) {
            return;//throw new RuntimeException("Could not find field " + clazz + "." + name + desc);
        }
        for (String annot : info.addClass(foundClazz).getField(name, desc).getAnnotations()) {
            processClass(annot);
        }
    }

    public void processMethod(String clazz, String name, String desc) {
        processClass(clazz);
        clazz = info.findDeclaringClassOfMethod(context, clazz, name, desc);
        if (clazz != null && !info.getClassInfo(clazz).isMethodReferenced(name, desc)) {
            info.getClassInfo(clazz).markMethodReferenced(name, desc);
            /*if (clazz.equals("io/netty/channel/socket/nio/NioSocketChannel") && name.equals("<init>")) {
                throw new RuntimeException(desc);
            }*/
            worklist.add(new WorkItem(clazz, name, desc));
            for (String annot : info.getClassInfo(clazz).getMethod(name, desc).getAnnotations()) {
                processClass(annot);
            }
            if (context.getServiceProviders(clazz) != null) {
                for (String c : context.getServiceProviders(clazz)) {
                    processMethod(c, name, desc);
                }
            }
        }
    }

    public void processVirtualMethod(String clazz, String name, String desc) {
        processMethod(clazz, name, desc);
        if (context.getSubClasses(clazz) != null) {
            for (String c : context.getSubClasses(clazz)) {
                if ((Config.inclusionMode == Config.INCLUSION_MODE_REFERENCE && info.isClassReferenced(c))
                    || (Config.inclusionMode == Config.INCLUSION_MODE_INSTANTIATE && info.isClassInstantiated(c))) {
                    processMethod(c, name, desc);
                }
            }
        }
    }

    public void processInterfaceMethod(String clazz, String name, String desc) {
        processMethod(clazz, name, desc);
        for (String i : context.getInterfaceImplementors(clazz)) {
            if ((Config.inclusionMode == Config.INCLUSION_MODE_REFERENCE && info.isClassReferenced(i))
                || (Config.inclusionMode == Config.INCLUSION_MODE_INSTANTIATE && info.isClassInstantiated(i))) {
                processMethod(i, name, desc);
            }
        }
    }
}
