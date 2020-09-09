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

import org.eclipse.openj9.jmin.info.ReferenceInfo;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.eclipse.openj9.jmin.util.WorkList;
import org.objectweb.asm.ClassVisitor;

public class BeanAnnotationProcessor extends AnnotationProcessor {
    public BeanAnnotationProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info, ClassVisitor next) {
        super(worklist, context, info, next);
        prefixes = new String[] { "Ljavax/", "Ljakarta/" };
        classAnnotations = new String[] {
            "annotation/ManagedBean",
            "enterprise/context/ApplicationScoped",
            "enterprise/context/SessionScoped",
            "enterprise/context/ConversationScoped",
            "enterprise/context/RequestScoped",
            "enterprise/context/NormalScope",
            "interceptor/Interceptor",
            "decorator/Decorator",
            "enterprise/context/Dependent",
            "enterprise/inject/Alternative",
            "inject/Singleton",
            "inject/Scope",
            "inject/Inject",
            "ws/rs/Produces",   
            "ws/rs/Consumes",
            "ws/rs/ext/Provider"
        };
        constructorAnnotations = new String[] {
            "inject/Inject"
        };
        methodAnnotations = new String[] {
            "enterprise/inject/Produces",
            "enterprise/inject/Specializes",
            "inject/Inject",
            "ws/rs/HttpMethod",
            "ws/rs/GET",
            "ws/rs/POST",
            "ws/rs/PUT",
            "ws/rs/DELETE",
            "ws/rs/PATCH",
            "ws/rs/Path",
            "annotation/PreDestroy",
            "annotation/PostConstruct",
            "inject/AroundInvoke"
        };
        methodParameterAnnotations = new String [] {
            "enterprise/event/Observes"
        };
        fieldAnnotations = new String[] {
            "enterprise/inject/Produces",
            "inject/Inject"
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
        if (context.getInterfaceImplementors("javax/ws/rs/container/DynamicFeature").contains(name)) {
            worklist.processMethod(name, "configure", "(Ljavax/ws/rs/container/ResourceInfo;Ljavax/ws/rs/core/FeatureContext;)V");
        }
        super.visit(version, access, name, signature, superName, interfaces);
    }
}