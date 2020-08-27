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

import org.eclipse.openj9.jmin.methodsummary.MethodSummary;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class MethodInfo {
    private final String clazz;
    private final String name;
    private final String desc;
    private final ArrayList<CallSite> callsites;
    private final ArrayList<CallSite> callers;
    private final HashSet<String> referencedClasses;
    private final ArrayList<FieldSite> referencedFields;
    private final HashSet<String> annotations;
    private final HashSet<String> instantiatedTypes;
    private MethodSummary summary;
    private boolean referenced;
    private boolean processed;
    private boolean processedForSummary;

    public MethodInfo(String clazz, String name, String desc) {
        this.clazz = clazz;
        this.name = name;
        this.desc = desc;
        this.callsites = new ArrayList<CallSite>();
        this.callers = new ArrayList<CallSite>();
        this.referencedClasses = new HashSet<String>();
        this.referencedFields = new ArrayList<FieldSite>();
        this.annotations = new HashSet<String>();
        this.instantiatedTypes = new HashSet<String>();
        this.summary = null;
        this.referenced = false;
        this.processed = false;
        this.processedForSummary = false;
    }
    public String clazz() { return clazz; }
    public String name() { return name; }
    public String desc() { return desc; }
    public void setReferenced() {
        referenced = true;
    }
    public void setProcessed() {
        this.processed = true;
    }
    public boolean referenced() {
        return this.referenced;
    }
    public boolean processed() {
        return this.processed;
    }
    public void addCallSite(String clazz, String name, String desc, CallKind kind, int instuctionIndex, ClassSource classSource) {
        callsites.add(new CallSite(this, clazz.replace('.', '/'), name, desc, kind, instuctionIndex, classSource));
    }
    public void addCaller(CallSite callSite) {
        callers.add(callSite);
    }
    public List<CallSite> getCallers() {
        assert ReferenceInfo.callersComputed : "Cannot access caller before it is computed";
        return callers;
    }
    public void addReferencedClass(String clazz) {
        referencedClasses.add(clazz.replace('.', '/'));
    }
    public void addReferencedField(String clazz, String name, String desc, FieldKind kind) {
        referencedFields.add(new FieldSite(clazz, name, desc, kind));
    }
    public List<CallSite> getCallSites() {
        return callsites;
    }
    public List<FieldSite> getReferencedFields() {
        return referencedFields;
    }
    public Set<String> getReferencedClasses() {
        return referencedClasses;
    }
    public void addAnnotation(String annot) {
        annotations.add(annot);
    }
    public HashSet<String> getAnnotations() {
        return annotations;
    }
    public void addInstantiatedClass(String clazz) {
        instantiatedTypes.add(clazz.replace('.', '/'));
    }
    public HashSet<String> getInstantiatedClasses() {
        return instantiatedTypes;
    }

    @Override
    public String toString() {
        return name + desc + " -" + (referenced ? "" : " not") + " referenced" + (processed ? "" : " not") + " processsed";
    }

    public MethodSummary getMethodSummary() {
        if (summary == null) {
            summary = new MethodSummary();
        }
        return summary;
    }
    public boolean hasMethodSummary() {
        return summary != null;
    }
    public void setProcessedForSummary() {
        processedForSummary = true;
    }
    public boolean isProcessedForSummary() { return processedForSummary; }
}