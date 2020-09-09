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

public class ApacheCXFPreProcessor extends PreProcessor {
    public ApacheCXFPreProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info) {
        super(worklist, context, info);
    }

    @Override
    public void process() {
        // Classes present in cxf-core.jar:META-INF/cxf/bus-extensions.txt
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/PhaseManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/WorkQueueManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/CXFBusLifeCycleManager");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/ServerRegistryImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/EndpointResolverRegistryImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/HeaderManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/service/factory/FactoryBeanListenerManager");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/ServerLifeCycleManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/managers/ClientLifeCycleManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/bus/resource/ResourceManagerImpl");
        worklist.forceInstantiateClass("org/apache/cxf/catalog/OASISCatalogManager");

        // Classes present in cxf-rt-rs-sse.jar:META-INF/cxf/bus-extensions.txt
        worklist.forceInstantiateClass("org/apache/cxf/transport/sse/SseProvidersExtension");

        // Classes present in cxf-rt-transports-http.jar:META-INF/cxf/bus-extensions.txt
        worklist.forceInstantiateClass("org/apache/cxf/transport/http/HTTPTransportFactory");
        worklist.forceInstantiateClass("org/apache/cxf/transport/http/HTTPWSDLExtensionLoader");
        worklist.forceInstantiateClass("org/apache/cxf/transport/http/policy/HTTPClientAssertionBuilder");
        worklist.forceInstantiateClass("org/apache/cxf/transport/http/policy/HTTPServerAssertionBuilder");
        worklist.forceInstantiateClass("org/apache/cxf/transport/http/policy/NoOpPolicyInterceptorProvider");
    }
}
