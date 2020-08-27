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

import java.util.Set;

import org.eclipse.openj9.jmin.info.ClassInfo;
import org.eclipse.openj9.jmin.info.MethodInfo;
import org.eclipse.openj9.jmin.info.ReferenceInfo;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.eclipse.openj9.jmin.util.WorkList;

public class HibernatePreProcessor extends PreProcessor {
    public HibernatePreProcessor(WorkList worklist, HierarchyContext context, ReferenceInfo info) {
        super(worklist, context, info);
    }
    public void process() {
        for (String svc : context.getInterfaceImplementors("org/hibernate/service/Service")) {
            worklist.forceInstantiateClass(svc);
            for (MethodInfo mi : info.getClassInfo(svc).getMethodsByNameOnly("<init>")) {
                worklist.processMethod(svc, "<init>", mi.desc());
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/service/spi/Startable");
            if (implementors.size() > 0) {
                for (String i : implementors) {
                    worklist.forceInstantiateClass(i);
                }
                worklist.processInterfaceMethod("org/hibernate/service/spi/Startable", "start", "()V");
            }
        }
        {
        Set<String> implementors = context.getInterfaceImplementors("org/hibernate/service/spi/Stoppable");
            if (implementors.size() > 0) {
                for (String i : implementors) {
                    worklist.forceInstantiateClass(i);
                }
                worklist.processInterfaceMethod("org/hibernate/service/spi/Stoppable", "stop", "()V");
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/service/spi/Wrapped");
            if (implementors.size() > 0) {
                worklist.processInterfaceMethod("org/hibernate/service/spi/Wrapped", "unwrap", "(Ljava/lang/Class;)Ljava/lang/Object;");
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/service/spi/Configurable");
            if (implementors.size() > 0) {
                for (String i : implementors) {
                    worklist.forceInstantiateClass(i);
                }
                worklist.processInterfaceMethod("org/hibernate/service/spi/Configurable", "configure", "(Ljava/util/Map;)V");
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/boot/registry/StandardServiceInitiator");
            if (implementors.size() > 0) {
                worklist.processInterfaceMethod("org/hibernate/boot/registry/StandardServiceInitiator", "initiateService", "(Ljava/util/Map;Lorg/hibernate/service/spi/ServiceRegistryImplementor;)Lorg/hibernate/service/Service;");
                for (String svcInit : implementors) {
                    worklist.forceInstantiateClass(svcInit);
                    for (MethodInfo mi : info.getClassInfo(svcInit).getMethodsByNameOnly("<init>")) {
                        worklist.processMethod(svcInit, "<init>", mi.desc());
                    }
                }
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/persister/spi/PersisterFactory");
            if (implementors.size() > 0) {
                worklist.processInterfaceMethod("org/hibernate/persister/spi/PersisterFactory", "createEntityPersister", "(Lorg/hibernate/mapping/PersistentClass;Lorg/hibernate/cache/spi/access/EntityDataAccess;Lorg/hibernate/cache/spi/access/NaturalIdDataAccess;Lorg/hibernate/persister/spi/PersisterCreationContext;)Lorg/hibernate/persister/entity/EntityPersister;");
                worklist.processInterfaceMethod("org/hibernate/persister/spi/PersisterFactory", "createCollectionPersister", "(Lorg/hibernate/mapping/Collection;Lorg/hibernate/cache/spi/access/CollectionDataAccess;Lorg/hibernate/persister/spi/PersisterCreationContext;)Lorg/hibernate/persister/collection/CollectionPersister;");
                for (String pf : implementors) {
                    worklist.forceInstantiateClass(pf);
                    for (MethodInfo mi : info.getClassInfo(pf).getMethodsByNameOnly("<init>")) {
                        worklist.processMethod(pf, "<init>", mi.desc());
                    }
                }
            }
        }
        {
            Set<String> implementors = context.getInterfaceImplementors("org/hibernate/id/enhanced/Optimizer");
            if (implementors.size() > 0) {
                for (String opt : implementors) {
                    worklist.forceInstantiateClass(opt);
                }
            }
        }
        // reflective creation from constructor in PersisterFactoryImpl
        for (String ef : context.getInterfaceImplementors("org/hibernate/persister/entity/EntityPersister")) {
            worklist.forceInstantiateClass(ef);
        }
        if (context.getSubClasses("io/quarkus/runtime/Application") != null) {
            for (String app : context.getSubClasses("io/quarkus/runtime/Application")) {
                worklist.forceInstantiateClass(app);
            }
        }
        //BeanValidationIntegrator - called by reflection
        ClassInfo tsa = info.getClassInfo("org/hibernate/cfg/beanvalidation/TypeSafeActivator");
        if (tsa != null) {
            for (MethodInfo mi : tsa.getMethodsByNameOnly("activate")) {
                worklist.processVirtualMethod("org/hibernate/cfg/beanvalidation/TypeSafeActivator", "activate", mi.desc());
            }
        
            for (MethodInfo mi : tsa.getMethodsByNameOnly("validateSuppliedFactory")) {
                worklist.processVirtualMethod("org/hibernate/cfg/beanvalidation/TypeSafeActivator", "validateSuppliedFactory", mi.desc());
            }
        }
        //Reflective instantiation in org/hibernate/tuple/component/ComponentTuplizerFactory.java based on the entityMode - preload all the implementations
        for (String i : context.getInterfaceImplementors("org/hibernate/tuple/Instantiator")) {
            worklist.forceInstantiateClass(i);
        }
        // unknown path
        worklist.forceInstantiateClass("org/hibernate/internal/SessionImpl$LobHelperImpl");
        // passed by map as hibernate.dialect in init
        worklist.forceInstantiateClass("io/quarkus/hibernate/orm/runtime/dialect/QuarkusPostgreSQL95Dialect");

        // instantiated on demand via calls to register in org.hibernate.id.factory.interal.DefaultIdentifierGeneratorFactory
        // issue here is that you need interprocedural detection of the classes being instantiated
        String[] idGenerators = new String[] {
            "org/hibernate/id/GUIDGenerator",
            "org/hibernate/id/IdentifierGenerator",
            "org/hibernate/id/ForeignGenerator",
            "org/hibernate/id/IdentityGenerator",
            "org/hibernate/id/IncrementGenerator",
            "org/hibernate/id/SelectGenerator",
            "org/hibernate/id/SequenceGenerator",
            "org/hibernate/id/SequenceHiLoGenerator",
            "org/hibernate/id/SequenceIdentityGenerator",
            "org/hibernate/id/UUIDGenerator",
            "org/hibernate/id/UUIDHexGenerator",
            "org/hibernate/id/enhanced/SequenceStyleGenerator",
            "org/hibernate/id/enhanced/TableGenerator",
        };
        for (String gen : idGenerators) {
            worklist.forceInstantiateClass(gen);
        }

        // instantiated on demand via calls to Class.newInstance in SqlASTFactory
        // need interprocedural detection of classes being instantiated
        String[] sqlASTTreeNodes = new String[] {
            "org/hibernate/hql/internal/ast/tree/AggregateNode",
            "org/hibernate/hql/internal/ast/tree/BetweenOperatorNode",
            "org/hibernate/hql/internal/ast/tree/BinaryArithmeticOperatorNode",
            "org/hibernate/hql/internal/ast/tree/BinaryLogicOperatorNode",
            "org/hibernate/hql/internal/ast/tree/BooleanLiteralNode",
            "org/hibernate/hql/internal/ast/tree/CastFunctionNode",
            "org/hibernate/hql/internal/ast/tree/NullNode",
            "org/hibernate/hql/internal/ast/tree/SearchedCaseNode",
            "org/hibernate/hql/internal/ast/tree/SimpleCaseNode",
            "org/hibernate/hql/internal/ast/tree/CollectionFunction",
            "org/hibernate/hql/internal/ast/tree/ConstructorNode",
            "org/hibernate/hql/internal/ast/tree/CountNode",
            "org/hibernate/hql/internal/ast/tree/DeleteStatement",
            "org/hibernate/hql/internal/ast/tree/DotNode",
            "org/hibernate/hql/internal/ast/tree/FromClause",
            "org/hibernate/hql/internal/ast/tree/FromElement",
            "org/hibernate/hql/internal/ast/tree/IdentNode",
            "org/hibernate/hql/internal/ast/tree/ImpliedFromElement",
            "org/hibernate/hql/internal/ast/tree/InLogicOperatorNode",
            "org/hibernate/hql/internal/ast/tree/IndexNode",
            "org/hibernate/hql/internal/ast/tree/InitializeableNode",
            "org/hibernate/hql/internal/ast/tree/InsertStatement",
            "org/hibernate/hql/internal/ast/tree/IntoClause",
            "org/hibernate/hql/internal/ast/tree/IsNotNullLogicOperatorNode",
            "org/hibernate/hql/internal/ast/tree/IsNullLogicOperatorNode",
            "org/hibernate/hql/internal/ast/tree/JavaConstantNode",
            "org/hibernate/hql/internal/ast/tree/LiteralNode",
            "org/hibernate/hql/internal/ast/tree/MapEntryNode",
            "org/hibernate/hql/internal/ast/tree/MapKeyNode",
            "org/hibernate/hql/internal/ast/tree/MapValueNode",
            "org/hibernate/hql/internal/ast/tree/MethodNode",
            "org/hibernate/hql/internal/ast/tree/OrderByClause",
            "org/hibernate/hql/internal/ast/tree/ParameterNode",
            "org/hibernate/hql/internal/ast/tree/QueryNode",
            "org/hibernate/hql/internal/ast/tree/ResultVariableRefNode",
            "org/hibernate/hql/internal/ast/tree/SelectClause",
            "org/hibernate/hql/internal/ast/tree/SelectExpressionImpl",
            "org/hibernate/hql/internal/ast/tree/SessionFactoryAwareNode",
            "org/hibernate/hql/internal/ast/tree/SqlFragment",
            "org/hibernate/hql/internal/ast/tree/SqlNode",
            "org/hibernate/hql/internal/ast/tree/UnaryArithmeticNode",
            "org/hibernate/hql/internal/ast/tree/UnaryLogicOperatorNode",
            "org/hibernate/hql/internal/ast/tree/UpdateStatement",
        };
        for (String node : sqlASTTreeNodes) {
            worklist.forceInstantiateClass(node);
        }
    }
}