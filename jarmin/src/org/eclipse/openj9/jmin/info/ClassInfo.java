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

import java.util.*;

public class ClassInfo {
    private final String name;
    private final HashMap<String, MethodInfo> methodInfo;
    private final HashMap<String, FieldInfo> fieldInfo;
    private final HashSet<String> annotations;
    boolean referenced;
    boolean instantiated;
    public ClassInfo(String name) {
        this.name = name;
        this.methodInfo = new HashMap<String, MethodInfo>();
        this.fieldInfo = new HashMap<String, FieldInfo>();
        this.annotations = new HashSet<String>();
    }
    public List<MethodInfo> getMethodInfoList() { return new ArrayList<>(methodInfo.values()); }
    public String name() {
        return name;
    }
    public MethodInfo addMethod(String name, String desc) {
        String key = methodKey(name, desc);
        if (!methodInfo.containsKey(key)) {
            methodInfo.put(key, new MethodInfo(this.name, name, desc));
        }
        return methodInfo.get(key);
    }
    public MethodInfo getMethod(String name, String desc) {
        return methodInfo.get(methodKey(name, desc));
    }
    public void setReferenced() {
        referenced = true;
    }
    public boolean isReferenced() {
        return referenced;
    }
    public void setInstantiated() {
        instantiated = true;
    }
    public boolean isInstantiated() {
        return instantiated;
    }
    private String fieldKey(String name, String desc) {
        return name + desc;
    }
    public FieldInfo addField(String name, String desc) {
        String key = fieldKey(name, desc);
        if (!fieldInfo.containsKey(key)) {    
            fieldInfo.put(key, new FieldInfo(name, desc));
        }
        return fieldInfo.get(key);
    }
    public FieldInfo getField(String name, String desc) {
        return fieldInfo.get(fieldKey(name, desc));
    }
    public boolean hasField(String name, String desc) {
        return fieldInfo.containsKey(fieldKey(name, desc));
    }
    public Set<FieldInfo> getFields() {
        return new HashSet<FieldInfo>(fieldInfo.values());
    }
    public void markFieldReferenced(String name, String desc) {
        fieldInfo.get(fieldKey(name, desc)).setReferenced();
    }
    public boolean isFieldReferenced(String name, String desc) {
        String key = name + desc;
        if (fieldInfo.containsKey(key)) {
            return fieldInfo.get(key).referenced();
        }
        return false;
    }
    private String methodKey(String name, String desc) {
        return name + desc;
    }
    public boolean hasMethod(String name, String desc) {
        return methodInfo.containsKey(methodKey(name, desc));
    }
    public void markMethodReferenced(String name, String desc) {
        String key = methodKey(name, desc);
        if (!methodInfo.containsKey(key)) {
            methodInfo.put(key, new MethodInfo(this.name, name, desc));
        }
        methodInfo.get(key).setReferenced();
    }
    public void markMethodProcessed(String name, String desc) {
        String key = methodKey(name, desc);
        if (!methodInfo.containsKey(key)) {
            methodInfo.put(key, new MethodInfo(this.name, name, desc));
        }
        methodInfo.get(key).setProcessed();
    }
    public Set<MethodInfo> getMethodsByNameOnly(String name) {
        HashSet<MethodInfo> toReturn = new HashSet<MethodInfo>();
        for (MethodInfo mi : methodInfo.values()) {
            if (mi.name().equals(name)) {
                toReturn.add(mi);
            }
        }
        return toReturn;
    }
    public boolean isMethodReferenced(String name, String desc) {
        MethodInfo info = methodInfo.get(methodKey(name, desc));
        return info != null && info.referenced();
    }
    public boolean isMethodProcessed(String name, String desc) {
        MethodInfo info = methodInfo.get(methodKey(name, desc));
        return info != null && info.processed();
    }
    public HashSet<MethodInfo> getMethods() {
        return new HashSet<MethodInfo>(methodInfo.values());
    }
    public Set<MethodInfo> getReferencedMethods() {
        HashSet<MethodInfo> toReturn = new HashSet<MethodInfo>();
        for (MethodInfo mi : methodInfo.values()) {
            if (mi.referenced()) {
                toReturn.add(mi);
            }
        }
        return toReturn;
    }
    public Set<FieldInfo> getReferencedFields() {
        HashSet<FieldInfo> toReturn = new HashSet<FieldInfo>();
        for (FieldInfo fi : fieldInfo.values()) {
            if (fi.referenced()) {
                toReturn.add(fi);
            }
        }
        return toReturn;
    }
    public void addAnnotation(String annot) {
        annotations.add(annot);
    }
    public Set<String> getAnnotations() {
        return annotations;
    }
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Class " + name + "\n  Fields:\n");
        for (String field : fieldInfo.keySet()) {
            if (fieldInfo.get(field).referenced()) {
                sb.append("    ");
                sb.append(field);
                sb.append("\n");
            }
        }
        sb.append("  Methods:\n");
        for (String method : methodInfo.keySet()) {
            MethodInfo mi = methodInfo.get(method);
            if (mi.referenced()) {
                sb.append("    ");
                sb.append(mi.name());
                sb.append(mi.desc());
                sb.append("\n");
            }
        }
        return sb.toString();
    }
}