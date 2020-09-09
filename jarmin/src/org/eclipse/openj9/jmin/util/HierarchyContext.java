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

package org.eclipse.openj9.jmin.util;

import org.eclipse.openj9.jmin.info.ClassSource;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class HierarchyContext {
    private boolean closureComputed;
    private JarMap clazzToJar;
    private ServiceMap serviceClazzMap;
    private HashMap<String, String[]> classInterfaces;
    private ImplementorMap interfaceImplementorMap;
    private SubMap subMap;
    private SuperMap superMap;
    private HashSet<String> runtimeAnnotations;

    public HierarchyContext() {
        clazzToJar = new JarMap();
        serviceClazzMap = new ServiceMap();
        interfaceImplementorMap = new ImplementorMap();
        subMap = new SubMap();
        superMap = new SuperMap();
        runtimeAnnotations = new HashSet<String>();
        classInterfaces = new HashMap<String, String[]>();
    }

    public boolean addClassToJarMapping(String clazz, ClassSource source) {
        assert !closureComputed : "Cannot alter the hierarchy after computing the closure of data structures for quering";
        if (!clazzToJar.containsKey(clazz)) {
            clazzToJar.put(clazz, source);
            return true;
        }
        return false;
    }

    public String getJarForClass(String clazz) {
        return clazzToJar.get(clazz).getJarFile();
    }

    public ClassSource getSourceForClass(String clazz) {
        return clazzToJar.get(clazz);
    }

    public Set<String> seenClasses() {
        return clazzToJar.keySet();
    }

    public void addServiceMap(String service, String clazz) {
        assert !closureComputed : "Cannot alter the hierarchy after computing the closure of data structures for quering";
        if (!serviceClazzMap.containsKey(service)) {
            serviceClazzMap.put(service, new ArrayList<String>());
        }
        serviceClazzMap.get(service).add(clazz);
    }

    public List<String> getServiceProviders(String service) {
        assert closureComputed : "Cannot call for hierarchy information before closure computation is complete";
        return serviceClazzMap.get(service);
    }

    public Set<String> getServiceInterfaces() {
        assert closureComputed : "Cannot call for hierarchy information before closure computation is complete";
        return serviceClazzMap.keySet();
    }

    public void addSuperClass(String clazz, String sup) {
        assert !closureComputed : "Cannot alter the hierarchy after computing the closure of data structures for quering";
        ArrayList<String> superclasses = new ArrayList<String>();
        if (sup != null) {
            superclasses.add(sup);
        }
        superMap.put(clazz, superclasses);
    }

    public List<String> getSuperClasses(String clazz) {
        if (superMap.containsKey(clazz)) {
            return superMap.get(clazz);
        }
        return java.util.Collections.emptyList();
    }

    public void addInterfaces(String clazz, String[] interfaces) {
        assert !closureComputed : "Cannot alter the hierarchy after computing the closure of data structures for quering";
        classInterfaces.put(clazz, interfaces);
    }

    private static String[] EMPTY_STR_ARRAY = new String[0];
    public String[] getClassInterfaces(String clazz) {
        if (classInterfaces.containsKey(clazz)) {
            return classInterfaces.get(clazz);
        }
        return EMPTY_STR_ARRAY;
    }

    public Set<String> getInterfaceImplementors(String clazz) {
        assert closureComputed : "Cannot call for hierarchy information before closure computation is complete";
        if (interfaceImplementorMap.containsKey(clazz)) {
            return interfaceImplementorMap.get(clazz);
        }
        return java.util.Collections.emptySet();
    }

    public void addRuntimeAnnotation(String clazz) {
        assert !closureComputed : "Cannot alter the hierarchy after computing the closure of data structures for quering";
        runtimeAnnotations.add(clazz);
    }

    public boolean hasRuntimeAnnotation(String clazz) {
        assert closureComputed : "Cannot call for hierarchy information before closure computation is complete";
        return runtimeAnnotations.contains(clazz);
    }

    public Set<String> getSubClasses(String clazz) {
        assert closureComputed : "Cannot call for hierarchy information before closure computation is complete";
        if (subMap.containsKey(clazz)) {
            return subMap.get(clazz);
        }
        return null;
    }

    public void computeClosure() {
        assert !closureComputed : "Closure can only be computed once";
        // construct closure of superclasses
        for (String c : superMap.keySet()) {
            ArrayList<String> itr = superMap.get(c);
            if (itr != null && itr.size() > 0) {
                for (itr = superMap.get(itr.get(0)); itr != null && itr.size() > 0; itr = superMap.get(itr.get(0))) {
                    superMap.get(c).addAll(itr);
                    if (itr.size() > 1)
                        break;
                }
            }
        }
        for (String c : superMap.keySet()) {
            for (String s : superMap.get(c)) {
                if (!subMap.containsKey(s)) {
                    subMap.put(s, new HashSet<String>());
                }
                subMap.get(s).add(c);
            }
        }
        //System.out.println("Number of subclasses of Object: " + subMap.get("java/lang/Object").size());

        // construct closure of interfaces
        for (String c : superMap.keySet()) {
            for (String s : superMap.get(c)) {
                if (classInterfaces.get(s) != null) {
                    for (String i : classInterfaces.get(s)) {
                        if (!interfaceImplementorMap.containsKey(i)) {
                            interfaceImplementorMap.put(i, new HashSet<String>());
                        }
                        interfaceImplementorMap.get(i).add(c);
                    }
                }
            }
            if (classInterfaces.get(c) != null) {
                for (String i : classInterfaces.get(c)) {
                    if (!interfaceImplementorMap.containsKey(i)) {
                        interfaceImplementorMap.put(i, new HashSet<String>());
                    }
                    interfaceImplementorMap.get(i).add(c);
                }
            }
        }

        for (String i : interfaceImplementorMap.keySet()) {
            if (classInterfaces.containsKey(i)) {
                for (String s : classInterfaces.get(i)) {
                    interfaceImplementorMap.get(s).addAll(interfaceImplementorMap.get(i));
                }
            }
        }
        closureComputed = true;
    }
}
