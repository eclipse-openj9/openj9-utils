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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

import org.eclipse.openj9.jmin.util.HierarchyContext;

import static org.eclipse.openj9.jmin.info.CallKind.INTERFACE;
import static org.eclipse.openj9.jmin.info.CallKind.VIRTUAL;

public class ReferenceInfo {
    private final HashMap<String, ClassInfo> classInfo;
    private final ArrayList<MethodInfo> reflectionCallers;
    private final HashMap<String, LinkedList<MethodInfo>> overriddenMethods;
    public static boolean callersComputed = false;

    public ReferenceInfo() {
        classInfo = new HashMap<String, ClassInfo>();
        reflectionCallers = new ArrayList<MethodInfo>();
        overriddenMethods = new HashMap<String, LinkedList<MethodInfo>>();
    }

    public List<ClassInfo> getClassInfoList() {
        return new ArrayList<>(classInfo.values());
    }

    public ClassInfo addClass(String name) {
        if (!classInfo.containsKey(name)) {
            classInfo.put(name, new ClassInfo(name));
        }
        return classInfo.get(name);
    }

    public boolean hasClass(String name) {
        return classInfo.containsKey(name);
    }

    public boolean isClassReferenced(String name) {
        ClassInfo info = classInfo.get(name);
        return info != null && info.isReferenced();
    }

    public boolean isClassInstantiated(String name) {
        ClassInfo info = classInfo.get(name);
        return info != null && info.isInstantiated();
    }

    public ClassInfo getClassInfo(String name) {
        return classInfo.get(name);
    }

    public int getReferencedClassCount() {
        int toReturn = 0;
        for (String c : classInfo.keySet()) {
            if (classInfo.get(c).isReferenced()) {
                toReturn++;
            }
        }
        return toReturn;
    }

    public int getClassCount() {
        return classInfo.size();
    }

    public String markFieldReference(HierarchyContext context, String clazz, String name, String desc) {
        ClassInfo ci = classInfo.get(clazz);
        String foundClazz = null;
        if (ci != null && ci.hasField(name, desc)) {
            foundClazz = clazz;
            ci.markFieldReferenced(name, desc);
        } else {
            for (String superClass : context.getSuperClasses(clazz)) {
                ci = classInfo.get(superClass);
                if (ci != null && ci.hasField(name, desc)) {
                    foundClazz = superClass;
                    ci.markFieldReferenced(name, desc);
                    break;
                }
            }
        }
        return foundClazz;
    }

    private ClassInfo findClassOfMethod(HierarchyContext context, String clazz, String name, String desc) {
        ClassInfo ci = classInfo.get(clazz);
        ClassInfo foundClazz = null;
        if (ci != null && ci.hasMethod(name, desc)) {
            foundClazz = ci;
        } else {
            LinkedList<String> classesToCheck = new LinkedList<String>();
            classesToCheck.addAll(context.getSuperClasses(clazz));

            while (!classesToCheck.isEmpty()) {
                String c = classesToCheck.pop();
                ci = classInfo.get(c);
                if (ci != null && ci.hasMethod(name, desc)) {
                    foundClazz = ci;
                    break;
                }
                String[] interfaces = context.getClassInterfaces(c);
                for (int i = 0; i < interfaces.length; ++i) {
                    classesToCheck.add(interfaces[i]);
                }
            }
        }
        return foundClazz;
    }

    public String findDeclaringClassOfMethod(HierarchyContext context, String clazz, String name, String desc) {
        ClassInfo ci = findClassOfMethod(context, clazz, name, desc);
        return ci == null ? null : ci.name();
    }

    public MethodInfo findMethodInfoForMethod(HierarchyContext context, String clazz, String name, String desc) {
        ClassInfo ci = findClassOfMethod(context, clazz, name, desc);
        if (ci != null) {
            return ci.getMethod(name, desc);
        } else {
            return null;
        }
    }

    public void forEachSubClass(HierarchyContext context, String clazz, ClassInfoVisitor visitor) {
        if (context.getSubClasses(clazz) != null) {
            for (String subClass : context.getSubClasses(clazz)) {
                visitor.visitClass(getClassInfo(subClass));
            }
        }
    }

    public void forEachInterfaceImplementor(HierarchyContext context, String intf, ClassInfoVisitor visitor) {
        for (String implementor : context.getInterfaceImplementors(intf)) {
            visitor.visitClass(getClassInfo(implementor));
        }
    }

    public void addReflectionCaller(MethodInfo minfo) {
        reflectionCallers.add(minfo);
    }

    public ArrayList<MethodInfo> getReflectionCallers() {
        return reflectionCallers;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (String clazz : classInfo.keySet()) {
            sb.append(classInfo.get(clazz).toString());
            sb.append("\n");
        }
        return sb.toString();
    }

    private String createKey(String calleeClass, String calleeName, String calleeDesc) {
        return calleeClass+calleeName+calleeDesc;
    }

    public LinkedList<MethodInfo> getOverriddenMethods(String calleeClass, String calleeName, String calleeDesc) {
        String key = createKey(calleeClass, calleeName, calleeDesc);
        return overriddenMethods.get(key);
    }

    public void addToOverriddenMethodMap(String calleeClass, String calleeName, String calleeDesc, LinkedList<MethodInfo> overriddenMethodsList) {
        String key = createKey(calleeClass, calleeName, calleeDesc);
        overriddenMethods.put(key, overriddenMethodsList);
    }

    private void addCallerToMethodInfo(CallSite callSite, LinkedList<MethodInfo> calleeList) {
        for (MethodInfo m: calleeList) {
            m.addCaller(callSite);
        }
    }

    private void processCallSite(HierarchyContext context, CallSite callSite, String calleeClass, String calleeName, String calleeDesc) {
        LinkedList<MethodInfo> calleeList = getOverriddenMethods(calleeClass, calleeName, calleeDesc);
        if (calleeList == null) {
            MethodInfo callee = findMethodInfoForMethod(context, calleeClass, calleeName, calleeDesc);
            if (callee != null) {
                final LinkedList<MethodInfo> overriddenList = new LinkedList<MethodInfo>();
                overriddenList.add(callee);
                if (callSite.kind == VIRTUAL || callSite.kind == INTERFACE) {
                    ClassInfoVisitor visitor = clazz -> {
                        MethodInfo m = clazz.getMethod(calleeName, calleeDesc);
                        if (m != null) {
                            overriddenList.add(m);
                        }
                    };
                    if (callSite.kind == VIRTUAL) {
                        forEachSubClass(context, calleeClass, visitor);
                    } else if (callSite.kind == INTERFACE) {
                        forEachInterfaceImplementor(context, calleeClass, visitor);
                    }
                }
                addToOverriddenMethodMap(calleeClass, calleeName, calleeDesc, overriddenList);
                addCallerToMethodInfo(callSite, overriddenList);
            }
        } else {
            addCallerToMethodInfo(callSite, calleeList);
        }
    }

    private void processCallSites(HierarchyContext context, MethodInfo minfo) {
        for (CallSite callSite: minfo.getCallSites()) {
            if (callSite.desc.equals("*")) {
                ClassInfo ci = getClassInfo(callSite.clazz);
                if (ci != null) {
                    for (MethodInfo mi : ci.getMethodsByNameOnly(callSite.name)) {
                        processCallSite(context, callSite, callSite.clazz, callSite.name, mi.desc());
                    }
                }
            } else {
                processCallSite(context, callSite, callSite.clazz, callSite.name, callSite.desc);
            }
        }
    }

    public void createCalleeCallerMap(HierarchyContext context) {
        assert !callersComputed : "Callers can only be computed once";
        System.out.println("Start creating caller list");
        for (ClassInfo clazz : getClassInfoList() ) {
            for (MethodInfo minfo: clazz.getMethodInfoList()) {
                processCallSites(context, minfo);
            }
        }
        callersComputed = true;
    }
}