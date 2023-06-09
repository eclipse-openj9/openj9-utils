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

package org.eclipse.openj9.jmin.methodsummary;

import org.eclipse.openj9.jmin.analysis.*;
import org.eclipse.openj9.jmin.analysis.ClassValue;
import org.eclipse.openj9.jmin.info.*;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.objectweb.asm.tree.analysis.BasicValue;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class Summarizer {
    private List<MethodInfo> methodsToSummarize;
    private HashSet<MethodInfo> methodsToSummarizeSet;
    private final CallSiteArgumentAnalyzer callSiteArgumentAnalyzer;

    public static void createSummary(ReferenceInfo info, HierarchyContext context) {
        Summarizer summarizer = new Summarizer(info, context);
        summarizer.summarize();
    }

    public Summarizer(ReferenceInfo info, HierarchyContext context) {
        methodsToSummarize = new ArrayList<MethodInfo>();
        methodsToSummarizeSet = new HashSet<MethodInfo>();
        for (MethodInfo m: info.getReflectionCallers()) {
            if (!methodsToSummarizeSet.contains(m)) {
                methodsToSummarize.add(m);
                methodsToSummarizeSet.add(m);
            }
        }
        callSiteArgumentAnalyzer = new CallSiteArgumentAnalyzer(context);
    }

    public void summarize() {
        while (methodsToSummarize.size() > 0) {
            MethodInfo minfo = methodsToSummarize.remove(0);
            if (!minfo.isProcessedForSummary()) {
                propagateSummary(minfo);
            }
        }
    }

    public void propagateSummary(MethodInfo minfo) {
        if (minfo.getMethodSummary().getInstantiatedValues() != null) {
            for (CallSite call: minfo.getCallers()) {
                if (call.kind == CallKind.DYNAMIC || call.desc.equals("*")) {
                    continue;
                }
                MethodInfo caller = call.getCaller();
                if (caller.equals(minfo) || caller.isProcessedForSummary()) {
                    continue;
                }
                if (call.getAllArgValues() == null) {
                    if (!callSiteArgumentAnalyzer.analyze(caller)) {
                        continue;
                    }
                }
                for (BasicValue instantiatedValue : minfo.getMethodSummary().getInstantiatedValues()) {
                    String className = null;
                    if (instantiatedValue instanceof ParameterValue) {
                        ParameterValue parameter = (ParameterValue) instantiatedValue;
                        BasicValue argument = call.getArgValue(parameter.getIndex());
                        if (argument instanceof ParameterValue) {
                            caller.getMethodSummary().addInstantiatedValue(argument);
                        } else if (argument instanceof StringBuilderValue) {
                            StringBuilderValue sbv = (StringBuilderValue) argument;
                            if (sbv.isComputable()) {
                                StringValue value = (StringValue) sbv.getContents();
                                className = value.getContents();
                                if (!caller.getInstantiatedClasses().contains(className)) {
                                    caller.addInstantiatedClass(className);
                                    System.out.println("Adding " + className + " to instantiated list of " +
                                            caller.clazz() + " " + caller.name() + " " + caller.desc());
                                } else {
                                    System.out.println(className + " already exists in instantiated list");
                                }
                            } else {
                                caller.getMethodSummary().addInstantiatedValue(sbv);
                            }
                        } else if (argument instanceof StringValue) {
                            className = ((StringValue) argument).getContents();
                            if (!caller.getInstantiatedClasses().contains(className)) {
                                caller.addInstantiatedClass(className);
                                System.out.println("Adding " + className + " to instantiated list of " +
                                        caller.clazz() + " " + caller.name() + " " + caller.desc());
                            } else {
                                System.out.println(className + " already exists in instantiated list");
                            }
                        } else if (argument instanceof ClassValue) {
                            className = ((ClassValue) argument).getName();
                            if (!caller.getInstantiatedClasses().contains(className)) {
                                caller.addInstantiatedClass(className);
                                System.out.println("Adding " + className + " to instantiated list of " +
                                        caller.clazz() + " " + caller.name() + " " + caller.desc());
                            } else {
                                System.out.println(className + " already exists in instantiated list");
                            }
                        }
                    } else if (instantiatedValue instanceof StringBuilderValue) {
                        StringBuilderValue sbv = ((StringBuilderValue) instantiatedValue).copyWithParamSubstituition(call.getAllArgValues());
                        if (sbv != null) {
                            if (sbv.isComputable()) {
                                StringValue value = (StringValue) sbv.getContents();
                                className = value.getContents();
                                if (!caller.getInstantiatedClasses().contains(className)) {
                                    caller.addInstantiatedClass(className);
                                    System.out.println("Adding " + className + " to instantiated list of " +
                                            caller.clazz() + " " + caller.name() + " " + caller.desc());
                                } else {
                                    System.out.println(className + " already exists in instantiated list");
                                }
                            } else {
                                caller.getMethodSummary().addInstantiatedValue(sbv);
                            }
                        }
                    }
                }
                if (caller.hasMethodSummary() && !methodsToSummarizeSet.contains(caller)) {
                    methodsToSummarize.add(caller);
                    methodsToSummarizeSet.add(caller);
                }
            }
        }
        minfo.setProcessedForSummary();
    }
}
