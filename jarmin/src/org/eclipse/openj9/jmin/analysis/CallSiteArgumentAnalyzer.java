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

import org.eclipse.openj9.jmin.info.*;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.objectweb.asm.Type;
import org.objectweb.asm.tree.MethodNode;
import org.objectweb.asm.tree.analysis.AnalyzerException;
import org.objectweb.asm.tree.analysis.BasicValue;
import org.objectweb.asm.tree.analysis.Frame;

import java.util.HashMap;

public class CallSiteArgumentAnalyzer {
    private MethodNodeCache nodeCache;

    public CallSiteArgumentAnalyzer(HierarchyContext context) {
        nodeCache = new MethodNodeCache(context);
    }

    public boolean analyze(MethodInfo minfo) {
        /* There can be multiple versions of a class, and hence its methods, in an application,
         * but we create only one MethodInfo representing all versions of a method of a class,
         * i.e. one-to-many relationship between MethodInfo and methods on disk.
         * The callsites in each version of the method are added to the same MethodInfo.
         * To find argument values of a callsite, we need to get the MethodNode which is unique
         * for the method version corresponding to that callsite.
         * To avoid analyzing a MethodNode multiple times (as there can be multiple callsites in a
         * version of the method), we first create a map of method source (actually class source) to
         * analysis frames.
         */
        HashMap<ClassSource, Frame[]> sourceToFramesMap = new HashMap<ClassSource, Frame[]>();
        for (CallSite callSite: minfo.getCallSites()) {
            MethodNode methodNode = nodeCache.getMethodNode(minfo, callSite.getClassSource());
            InterospectiveAnalyzer a = new InterospectiveAnalyzer(new ReflectionInterpreter());
            try {
                a.analyze(minfo.clazz(), methodNode);
            } catch (AnalyzerException e) {
                return false;
            }
            sourceToFramesMap.put(callSite.getClassSource(), a.getFrames());
        }
        /* Now for each callsite, we can get the analysis frames by using callsite's class source
         * as the key to the map created above.
         */
        for (CallSite callSite : minfo.getCallSites()) {
            Frame[] frames = sourceToFramesMap.get(callSite.getClassSource());
            if (callSite.kind == CallKind.DYNAMIC || callSite.desc.equals("*")) {
                continue;
            }
            assert frames.length >= callSite.getInstructionIndex() : frames.length + " " + callSite.getInstructionIndex();
            Frame<BasicValue> frame = frames[callSite.getInstructionIndex()];
            if (frame != null) {
                BasicValue[] argValueList = new BasicValue[Type.getArgumentTypes(callSite.desc).length];
                int top = frame.getStackSize() - 1;
                for (int argIndex = 0; argIndex < argValueList.length; argIndex++) {
                    argValueList[argIndex] = frame.getStack(top - (argValueList.length - argIndex - 1));
                }
                callSite.setArgValueList(argValueList);
            }
        }
        return true;
    }
}
