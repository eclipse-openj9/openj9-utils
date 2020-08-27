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

public class QuarkusPreProcessor extends PreProcessor {
    private static String[] interfaces = new String[] {
        "io/quarkus/arc/InjectableBean",
        "io/quarkus/arc/InjectableContext",
        "io/quarkus/arc/InjectableInstance",
        "io/quarkus/arc/InjectableInterceptor",
        "io/quarkus/arc/InjectableObserverMethod",
        "io/quarkus/arc/InjectableReferenceProvider"
    };
    
    public QuarkusPreProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info) {
        super(worklist, context, info);
    }
    public void process() {
        for (String i : interfaces) {
            for (String c : context.getInterfaceImplementors(i)) {
                worklist.forceInstantiateClass(c);
                for (MethodInfo mi : info.getClassInfo(c).getMethodsByNameOnly("<init>")) {
                    worklist.processMethod(c, mi.name(), mi.desc());
                }
            }
        }
    }
}