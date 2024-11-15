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

package differ;

import java.util.Arrays;
import java.util.List;
import java.util.Iterator;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.LinkedHashSet;

import soot.jimple.toolkits.callgraph.CallGraph;
import soot.jimple.toolkits.callgraph.ExplicitEdgesPred;
import soot.jimple.toolkits.callgraph.Filter;
import soot.jimple.toolkits.callgraph.Targets;

import soot.jimple.Constant;
import soot.jimple.IntConstant;
import soot.jimple.StringConstant;
import soot.Modifier;
import soot.Scene;
import soot.SootClass;
import soot.SootField;
import soot.SootMethod;
import soot.SootMethodRef;
import soot.Body;
import soot.Unit;
import soot.PatchingChain;
import soot.util.HashChain;
import soot.jimple.InvokeExpr;
import soot.jimple.SpecialInvokeExpr;
import soot.jimple.InstanceInvokeExpr;
import soot.jimple.InstanceFieldRef;
import soot.jimple.Stmt;
import soot.jimple.AssignStmt;
import soot.jimple.DefinitionStmt;
import soot.jimple.NopStmt;
import soot.Type;
import soot.BooleanType;
import soot.VoidType;
import soot.Local;
import soot.ValueBox;
import soot.Value;
import soot.RefType;
import soot.util.Chain;
import soot.jimple.Jimple;
import soot.jimple.JimpleBody;
import soot.jimple.StaticInvokeExpr;
import soot.jimple.ThisRef;

public class PatchTransformer {

    // might need refactor if there is not a 1:1 map
    private HashMap<SootClass, SootClass> redefToNewClassMap;
    private HashMap<SootClass, SootClass> newToRedefClassMap;
    private Filter explicitInvokesFilter;
    private HashMap<SootField, SootMethod> fieldToGetter = new HashMap<SootField, SootMethod>();
    private HashMap<SootField, SootMethod> fieldToSetter = new HashMap<SootField, SootMethod>();
    private HashMap<SootField, SootField> oldFieldToNew = new HashMap<SootField, SootField>();

    private ArrayList<SootMethod> newMethods = new ArrayList<SootMethod>();

    private static String ogToHostHashTableName = "originalToHostHashTable";
    private static String hostToOgHashTableName = "hostToOriginalHashTable";

    public PatchTransformer(HashMap<SootClass, SootClass> newClassMap,
            HashMap<SootClass, SootClass> newClassMapReversed) {
        // just want a ref, to same structure in SemanticDiffer
        this.redefToNewClassMap = newClassMap;
        this.newToRedefClassMap = newClassMapReversed;
        explicitInvokesFilter = new Filter(new ExplicitEdgesPred());
    }

    public void moveMethodCalls(SootClass redefinition, List<SootMethod> addedMethods) {
        // remove the added methods from redefinition class
        // and place into wrapper class
        for (SootMethod m : addedMethods) {
            SootMethodRef oldRef = m.makeRef();
            redefinition.removeMethod(m);
            // dont want to move the static initializer, will construct it later in newclass
            if (!m.getName().equals("<clinit>")) {
                if (m.isPrivate()) {
                    // clear private, set protected
                    m.setModifiers((m.getModifiers() & (~Modifier.PRIVATE)) | Modifier.PROTECTED);
                }
                redefToNewClassMap.get(redefinition).addMethod(m);
                newMethods.add(m);
            }
            System.out.println("These are the new methods list: " + newMethods);
        }
    }

    // checks all of the provided methods for method refs that need fixing
    public void transformMethodCalls(List<SootMethod> methods, boolean isThisARedefClass) {
        // the annoying issue of adding a constructor to this class if needed, which is
        // determined while looping the methods
        Chain<SootMethod> methodsChain = new HashChain<SootMethod>();
        for (SootMethod m : methods) {
            methodsChain.add(m);
        }
        for (Iterator<SootMethod> iter = methodsChain.snapshotIterator(); iter.hasNext();) {
            SootMethod m = iter.next();
            findMethodCalls(m, isThisARedefClass);
        }
    }

    private void findMethodCalls(SootMethod m, boolean isThisARedefClass) {
        CallGraph cg = Scene.v().getCallGraph();
        Body body;
        System.out.println("Finding method calls in : " + m.getSignature());
        body = m.retrieveActiveBody();

        // need to be able to concurrently mod
        PatchingChain<Unit> units = body.getUnits();
        Iterator<Unit> it = units.snapshotIterator();

        while (it.hasNext()) {
            Unit u = it.next();
            Stmt s = (Stmt) u;
            Unit insnAfterInvokeInsn = units.getSuccOf(u);

            if (s.containsInvokeExpr()) {
                InvokeExpr invokeExpr = s.getInvokeExpr();
                System.out.println("Found a method call: " + invokeExpr.getMethodRef());
                System.out.println("Found in stmt: " + s);
                System.out.println("Printing the targets of this call: ");
                System.out.println(".....................................");
                Iterator targets = new Targets(explicitInvokesFilter.wrap(cg.edgesOutOf(s)));
                System.out.println(".....................................");
                if (targets.hasNext()) {
                    Iterator newTargets = new Targets(explicitInvokesFilter.wrap(cg.edgesOutOf(s)));
                    ArrayList<SootMethod> sortedTargets = sortTargets(newTargets);
                    boolean checkForMapped = checkForMapped(sortedTargets);

                    if (checkForMapped) {
                        // relying solely on targets in the cg, since method refs could have child
                        // classes that rely on parent defs
                        if (sortedTargets.size() == 1) {
                            SootMethod target = sortedTargets.get(0);
                            System.out.println("Replacing a method call in this statement: " + s);
                            System.out.println(invokeExpr.getMethodRef() + " ---> " + target.makeRef());
                            // condition where the cg has only one explicit target for this call
                            if (invokeExpr instanceof StaticInvokeExpr) {
                                // if its a static invoke we can just replace ref
                                invokeExpr.setMethodRef(target.makeRef());
                            } else {
                                // otherwise we gotta create a var of newClass type to invoke this on
                                // TODO make this safe... need to singleton the locals ugh
                                System.out.println("This is the redeftonewMap" + redefToNewClassMap);
                                System.out.println(
                                        "This is the target decl decl class: " + target.getDeclaringClass().getName());
                                SootClass newClass = target.getDeclaringClass();
                                constructNewCall(newClass, invokeExpr, units, u, body, isThisARedefClass);
                            }
                        } else {
                            // more than one target
                            for (SootMethod target : sortedTargets) {
                                System.out.println("This target is one we are fixing: " + target);
                                boolean lastCheck = (sortedTargets.indexOf(target) == sortedTargets.size() - 1) ? true
                                        : false;
                                constructGuard(target, body, units, insnAfterInvokeInsn, u, lastCheck);
                            }
                            System.out.println("Removing statement: " + u);
                            units.remove(u);
                            System.out.println("................................");
                        }

                    } else {
                        System.out.println("Did not do anything with this statment: " + s
                                + " because there were no relevant targets.");
                    }
                } else {
                    System.out.println(
                            "Did not do anything with this statment: " + s + " because there were no targets at all.");
                }
            }
            System.out.println("-------------------------------");
        }
    }

    // determines if any of the targets correspond to moved methods, only care about
    // changing calls that may have moved targets
    private boolean checkForMapped(ArrayList<SootMethod> sorted) {
        for (SootMethod target : sorted) {
            if (newMethods.contains(target)) {
                return true;
            }
        }
        return false;
    }

    // this is a dumb way to sort things, but since the targets are implemented as
    // iterator I think it needs to be this way
    private ArrayList<SootMethod> sortTargets(Iterator newTargets) {
        // map of parents to possibly multiple children
        HashMap<SootClass, LinkedHashSet<SootClass>> unsorted = new HashMap<SootClass, LinkedHashSet<SootClass>>();
        HashMap<SootClass, SootMethod> unsortedDeclToMethods = new HashMap<SootClass, SootMethod>();
        // targets sorted in order of children to parents
        ArrayList<SootClass> sortedHierarchy = new ArrayList<SootClass>();
        ArrayList<SootMethod> sortedHierarchyMethods = new ArrayList<SootMethod>();
        SootClass object = Scene.v().getSootClass("java.lang.Object");

        while (newTargets.hasNext()) {
            SootMethod target = (SootMethod) newTargets.next();
            System.out.println("This is a target out of the iterator: " + target);
            if (target.getDeclaringClass() != object) {
                SootClass targetSuper = target.getDeclaringClass().getSuperclass();
                if (newToRedefClassMap.get(targetSuper) != null) {
                    targetSuper = newToRedefClassMap.get(targetSuper);
                }
                if (!unsorted.containsKey(targetSuper)) {
                    unsorted.put(targetSuper, new LinkedHashSet<SootClass>());
                }
                SootClass toAdd = target.getDeclaringClass();
                if (newToRedefClassMap.get(toAdd) != null) {
                    toAdd = newToRedefClassMap.get(toAdd);
                }
                unsorted.get(targetSuper).add(toAdd);
                unsortedDeclToMethods.put(toAdd, target);
            }
        }

        // start with known top of hierarchy
        SootClass index = object;
        traverseChildren(sortedHierarchy, unsorted, index);

        for (SootClass sootclass : sortedHierarchy) {
            sortedHierarchyMethods.add(unsortedDeclToMethods.get(sootclass));
        }

        System.out.println("This is the unsorted list: " + unsorted);
        System.out.println("This is the sorted hierarchy: " + sortedHierarchy);
        System.out.println("This is the sorted hierarchy of methods: " + sortedHierarchyMethods);
        return sortedHierarchyMethods;
    }

    private void traverseChildren(ArrayList<SootClass> sortedHierarchy,
            HashMap<SootClass, LinkedHashSet<SootClass>> unsorted, SootClass index) {
        if (unsorted.get(index) != null) {
            for (SootClass child : unsorted.get(index)) {
                sortedHierarchy.add(0, child);
            }
            for (SootClass child : unsorted.get(index)) {
                traverseChildren(sortedHierarchy, unsorted, child);
            }
        }
    }

    private void constructNewCall(SootClass newClass, InvokeExpr invokeExpr, PatchingChain<Unit> units,
            Unit currentInsn, Body body, boolean isThisARedefClass) {

        System.out.println("This is the newclass getType: " + newClass.getType());
        Local newClassRef;

        if (isThisARedefClass) {
            Value base = ((InstanceInvokeExpr) invokeExpr).getBase();
            newClassRef = lookup(newClass, newClass, body, units, currentInsn, base, ogToHostHashTableName);
        } else {
            newClassRef = body.getThisLocal();
        }

        SootMethodRef newClassMethod = newClass.getMethod(invokeExpr.getMethodRef().getSubSignature()).makeRef();
        System.out.println("Replacing a method call in this statement: " + (Stmt) currentInsn);
        System.out.println(invokeExpr.getMethodRef() + " ---> " + newClassMethod);
        if (currentInsn instanceof AssignStmt) {
            // invoke in an assignment can only be right op
            units.insertBefore(
                    Jimple.v().newAssignStmt(((AssignStmt) currentInsn).getLeftOp(),
                            Jimple.v().newVirtualInvokeExpr(newClassRef, newClassMethod, invokeExpr.getArgs())),
                    currentInsn);
        } else {
            units.insertBefore(
                    Jimple.v().newInvokeStmt(
                            Jimple.v().newVirtualInvokeExpr(newClassRef, newClassMethod, invokeExpr.getArgs())),
                    currentInsn);
        }
        System.out.println("Removing statement: " + currentInsn);
        units.remove(currentInsn);
    }

    /*
     * constructs a check on the runtime type of the object that a method is invoked
     * upon. if it matches type that we stole from, then execution goes to a new
     * block in program that calls the new method in its new class The checks are
     * instanceof AND they are ordered from CHILD -> PARENT
     *
     * original program: ` a.someMethod()`
     *
     * fixed program: ``` hostClass = mapRedefToNew[targetClass] || targetClass
     * if(baseVar instanceof hostClass): hostClass.someMethod() else if: //possibly
     * more checks
     *
     * ```
     *
     */
    private void constructGuard(SootMethod target, Body body, PatchingChain<Unit> units, Unit insnAfterInvokeInsn,
            Unit currentInsn, boolean lastCheck) {
        InvokeExpr invokeExpr = ((Stmt) currentInsn).getInvokeExpr();
        // already performed the move by now, so the current decl class is correct
        SootClass newClass = target.getDeclaringClass();
        // may or may not actually be a newclass, if it is, need to instanitate and also
        // know this for the instanceof check
        SootClass originalClass = newToRedefClassMap.get(newClass);
        Local invokeobj;
        Type checkType;

        Local base = (Local) ((InstanceInvokeExpr) invokeExpr).getBase();
        NopStmt nop = Jimple.v().newNopStmt();

        // create the check
        if (!lastCheck) {
            Local boolInstanceOf = Jimple.v().newLocal("boolInstanceOf", BooleanType.v());
            body.getLocals().add(boolInstanceOf);

            checkType = newClass.getType();
            if (originalClass != null) {
                checkType = originalClass.getType();
            }
            units.insertBefore(Jimple.v().newAssignStmt(boolInstanceOf, Jimple.v().newInstanceOfExpr(base, checkType)),
                    currentInsn);
            units.insertBefore(Jimple.v().newIfStmt(Jimple.v().newEqExpr(boolInstanceOf, IntConstant.v(0)), nop),
                    currentInsn);

            // create the new block that contains the call to the new method
            System.out.println("This is the newclass getType: " + newClass.getType());
        }

        // consideration: fix this for the methodrefs in the added methods, those should
        // just use "this" not new local
        if (originalClass == null) {

            // this is where we did not move
            invokeobj = Jimple.v().newLocal("baseinvokeobj", newClass.getType());
            body.getLocals().add(invokeobj);
            units.insertBefore(Jimple.v().newAssignStmt(invokeobj, Jimple.v().newCastExpr(base, newClass.getType())),
                    currentInsn);
            if (currentInsn instanceof AssignStmt) {
                units.insertBefore(
                        Jimple.v().newAssignStmt(((AssignStmt) currentInsn).getLeftOp(),
                                Jimple.v().newVirtualInvokeExpr(invokeobj, target.makeRef(), invokeExpr.getArgs())),
                        currentInsn);
            } else {
                units.insertBefore(
                        Jimple.v().newInvokeStmt(
                                Jimple.v().newVirtualInvokeExpr(invokeobj, target.makeRef(), invokeExpr.getArgs())),
                        currentInsn);
            }

        } else {

            invokeobj = lookup(newClass, newClass, body, units, currentInsn, base, ogToHostHashTableName);
            // build the new call
            if (currentInsn instanceof AssignStmt) {
                units.insertBefore(
                        Jimple.v().newAssignStmt(((AssignStmt) currentInsn).getLeftOp(),
                                Jimple.v().newVirtualInvokeExpr(invokeobj, target.makeRef(), invokeExpr.getArgs())),
                        currentInsn);
            } else {
                units.insertBefore(
                        Jimple.v().newInvokeStmt(
                                Jimple.v().newVirtualInvokeExpr(invokeobj, target.makeRef(), invokeExpr.getArgs())),
                        currentInsn);
            }

        }

        units.insertBefore(Jimple.v().newGotoStmt(insnAfterInvokeInsn), currentInsn);

        System.out.println("This is the method ref: " + invokeExpr.getMethodRef());
        System.out.println("This is the method ref declaring class: " + invokeExpr.getMethodRef().getDeclaringClass());

        // not sure if needed, might be dead code, but copied from what was in a jimple
        // example
        units.insertBefore(nop, currentInsn);

        System.out.println("This is the newToRedefClassMap: " + newToRedefClassMap);
    }

    public void transformFields(SootClass original, SootClass redefinition, List<SootField> addedFields,
            List<SootMethod> addedMethods) {
        for (SootField field : addedFields) {
            fixFields(field, redefinition);
        }

        // bit weird to put it here, but must be called before field removal
        fixClinit(redefinition);

        fixFieldRefs(redefinition, addedMethods);

        for (SootField field : addedFields) {
            System.out.println(
                    "Removing this field: " + field.getSignature() + " from this class: " + redefinition.getName());
            redefinition.removeField(field);
        }

        // fix the field refs for "the other way" - refs using "this" that refer to a
        // redef class, in an added method
        fixFieldRefsInAddedMethods(redefinition, addedMethods);
    }

    // adds the field to the new class and if the field was private constructs an
    // accessor for it
    private void fixFields(SootField field, SootClass redefinition) {
        // cant actually just move the same field ref, need it to exist in both classes
        // simultaneously in order to fix refs
        SootField newField = new SootField(field.getName(), field.getType(), field.getModifiers());

        if (newField.isPrivate()) {
            // clear private, set protected, so we can do bad direct access
            // TODO fix the later accesses to all be through the accessors
            newField.setModifiers((newField.getModifiers() & (~Modifier.PRIVATE)) | Modifier.PROTECTED);
        }

        SootClass newClass = redefToNewClassMap.get(redefinition);
        newClass.addField(newField);
        System.out
                .println("Adding this field: " + newField.getSignature() + " to this new class: " + newClass.getName());
        oldFieldToNew.put(field, newField);

        System.out.println("Is the newfield static?: " + newField.isStatic());
        System.out.println("Is the newfield phantom for some reason?:" + newField.isPhantom());

        // construct getter and setter for even public fields
        String methodName = "get" + field.getName();

        // getter first
        // need the acessor to be public , might need to also be static
        int modifiers = Modifier.PROTECTED;
        if (field.isStatic()) {
            modifiers |= Modifier.STATIC;
        }
        SootMethod newGetter = new SootMethod(methodName, Arrays.asList(new Type[] {}), field.getType(), modifiers);

        JimpleBody getterBody = Jimple.v().newBody(newGetter);
        newGetter.setActiveBody(getterBody);
        Chain units = getterBody.getUnits();

        // must create local then can return that
        Local tmpref = Jimple.v().newLocal("tmpref", field.getType());
        getterBody.getLocals().add(tmpref);
        if (field.isStatic()) {
            units.add(Jimple.v().newAssignStmt(tmpref, Jimple.v().newStaticFieldRef(newField.makeRef())));
        } else {
            // assign a local for self so we can ref our own field
            Local selfref = Jimple.v().newLocal("selfref", newClass.getType());
            getterBody.getLocals().add(selfref);
            units.add(Jimple.v().newIdentityStmt(selfref, new ThisRef(newClass.getType())));
            units.add(Jimple.v().newAssignStmt(tmpref, Jimple.v().newInstanceFieldRef(selfref, newField.makeRef())));
        }

        units.add(Jimple.v().newReturnStmt(tmpref));

        newClass.addMethod(newGetter);
        fieldToGetter.put(field, newGetter);

        // make setter too
        String setterMethodName = "set" + field.getName();
        // may have some issue here if there was casting for stmts we will replace...
        SootMethod newSetter = new SootMethod(setterMethodName, Arrays.asList(new Type[] { field.getType() }),
                VoidType.v(), modifiers);

        JimpleBody setterBody = Jimple.v().newBody(newSetter);
        newSetter.setActiveBody(setterBody);
        Chain setterUnits = setterBody.getUnits();
        // have to assign the param to a local first, cannot go directly from param to
        // assignstmt
        Local paramref = Jimple.v().newLocal("paramref", field.getType());
        setterBody.getLocals().add(paramref);
        setterUnits.add(Jimple.v().newIdentityStmt(paramref, Jimple.v().newParameterRef(field.getType(), 0)));
        if (field.isStatic()) {
            setterUnits.add(Jimple.v().newAssignStmt(Jimple.v().newStaticFieldRef(newField.makeRef()), paramref));
        } else {
            // assign a local for self so we can ref our own field
            Local selfref = Jimple.v().newLocal("selfref", newClass.getType());
            setterBody.getLocals().add(selfref);
            setterUnits.add(
                    Jimple.v().newAssignStmt(Jimple.v().newInstanceFieldRef(selfref, newField.makeRef()), paramref));
            setterUnits.addFirst(Jimple.v().newIdentityStmt(selfref, new ThisRef(newClass.getType())));
        }

        setterUnits.add(Jimple.v().newReturnVoidStmt());
        newClass.addMethod(newSetter);
        fieldToSetter.put(field, newSetter);
    }

    // fixes the static field value changes, for preexisting fields
    public void fixStaticFieldValueChanges(SootClass original, SootClass redefinition, List<SootField> addedFields) {
        System.out.println("Checking if all original fields' values match, in original: " + original.getName()
                + " and redef: " + redefinition.getName());
        boolean foundOne = false;
        for (SootField field : redefinition.getFields()) {
            if (!addedFields.contains(field) && field.isStatic()) {
                SootField originalfield = original.getField(field.getName(), field.getType());
                System.out.println(field.isStatic());
                System.out.println(addedFields.contains(field));
                System.out.println("---------------------------------------------------");
                System.out.println("Found a static original field, to do a value change check on. Original: "
                        + originalfield.getName() + " vs Redefinition: " + field.getName());

                SootMethod clinitRedef = redefinition.getMethodUnsafe("<clinit>", Arrays.asList(new Type[] {}),
                        VoidType.v());
                SootMethod clinitOriginal = original.getMethodUnsafe("<clinit>", Arrays.asList(new Type[] {}),
                        VoidType.v());

                if (clinitRedef != null && clinitOriginal != null) {

                    Iterator<Unit> redefit = clinitRedef.retrieveActiveBody().getUnits().snapshotIterator();
                    Iterator<Unit> originalit = clinitOriginal.retrieveActiveBody().getUnits().snapshotIterator();
                    Value redefDef = null;
                    Value originalDef = null;

                    while (redefit.hasNext()) {
                        Unit u = redefit.next();
                        Stmt s = (Stmt) u;
                        // if its the definition statement
                        if (s.containsFieldRef() && s.getFieldRef().getField().equals(field)
                                && !containsUse(u, field)) {
                            redefDef = ((AssignStmt) u).getRightOp();
                        }
                    }
                    while (originalit.hasNext()) {
                        Unit u = originalit.next();
                        Stmt s = (Stmt) u;
                        // if its the definition statement
                        if (s.containsFieldRef() && s.getFieldRef().getField().equals(originalfield)
                                && !containsUse(u, originalfield)) {
                            originalDef = ((AssignStmt) u).getRightOp();
                        }
                    }

                    System.out.println("---------------------------------------------------");
                    System.out.println("This is original value: " + originalDef + " of field: " + originalfield);
                    System.out.println("This is redef value: " + redefDef + " of field: " + field);

                    // this will get it moved with the other added field initialization movement
                    if (originalDef != null && redefDef != null && originalDef instanceof Constant
                            && redefDef instanceof Constant && !originalDef.equivTo(redefDef)) {
                        System.out.println("Found a value change in: " + originalDef + "--->" + redefDef);
                        // TODO fix this for private methods, would need to build a unsafe access...
                        oldFieldToNew.put(field, field);
                        foundOne = true;
                    }
                    System.out.println("---------------------------------------------------");

                    if (addedFields.size() == 0 && foundOne) {
                        fixClinit(redefinition);
                    }

                } else {
                    System.out.println("Found a static field with no matching clinits. Field: " + field.getName());
                    System.out.println("Original clinit: " + clinitOriginal + " and redef clinit: " + clinitRedef);
                }
            }

        }
    }

    private void fixClinit(SootClass redefinition) {
        List<Local> relevantLocals = new ArrayList<Local>();
        List<Unit> changeSet = new ArrayList<Unit>();
        List<Unit> relevantStmts = new ArrayList<Unit>();

        SootMethod clinitRedef = redefinition.getMethodUnsafe("<clinit>", Arrays.asList(new Type[] {}), VoidType.v());
        if (clinitRedef != null) {
            SootClass newClass = redefToNewClassMap.get(redefinition);
            createStaticInitializer(newClass);

            SootMethod clinitNew = newClass.getMethod("<clinit>", Arrays.asList(new Type[] {}), VoidType.v());
            Body clinitBody = clinitNew.retrieveActiveBody();
            PatchingChain<Unit> newUnits = clinitBody.getUnits();

            Body redefBody = clinitRedef.retrieveActiveBody();
            PatchingChain<Unit> redefUnits = redefBody.getUnits();
            Iterator<Unit> it = redefUnits.snapshotIterator();

            // first pass, just find the field refs for the moved fields first, and patch
            // those up while at it.
            while (it.hasNext()) {
                Unit u = it.next();
                Stmt s = (Stmt) u;
                if (s.containsFieldRef()) {
                    SootField ref = s.getFieldRef().getField();
                    SootField newref = oldFieldToNew.get(ref);
                    if (newref != null) {
                        s.getFieldRefBox().setValue(Jimple.v().newStaticFieldRef(oldFieldToNew.get(ref).makeRef()));
                        relevantStmts.add(u);
                        relevantLocals.addAll(extractLocals(u));
                    }
                }
            }

            changeSet.addAll(forwardPass(relevantLocals, relevantStmts, redefUnits));
            while (changeSet.size() != 0) { // loop until fixpoint
                changeSet.clear();
                changeSet.addAll(forwardPass(relevantLocals, relevantStmts, redefUnits));
            }

            // add an anchor nop
            NopStmt nop = Jimple.v().newNopStmt();
            newUnits.addFirst((Unit) nop);
            Unit ptr = (Unit) nop;

            // move locals first
            for (Local l : relevantLocals) {
                if (!clinitBody.getLocals().contains(l)) {
                    System.out.println("Moving this local: " + l);
                    clinitBody.getLocals().add(l);
                }
            }

            // second pass is for moviing
            Iterator<Unit> itTwo = redefUnits.snapshotIterator();
            while (itTwo.hasNext()) {
                Unit u = itTwo.next();
                if (relevantStmts.contains(u)) {
                    System.out.println("-----------------------------");
                    System.out.println("Moving this statement: " + u);
                    System.out.println("-----------------------------");
                    newUnits.insertAfter(u, ptr);
                    ptr = u;
                    redefUnits.remove(u);
                }
            }
        }
    }

    private void fixFieldRefs(SootClass redefinition, List<SootMethod> addedMethods) {
        for (SootMethod m : redefinition.getMethods()) {
            if (!m.getName().equals("<clinit>") && m.hasActiveBody()) {
                boolean addedMethod = false;
                if (addedMethods.contains(m)) {
                    addedMethod = true;
                    System.out.println("Finding field refs in an added method: " + m.getSignature());
                } else {
                    System.out.println("Finding field refs in: " + m.getSignature());
                }
                System.out.println("Method name is: " + m.getName());
                System.out.println("-------------------------------");
                Body body = m.retrieveActiveBody();
                PatchingChain<Unit> units = body.getUnits();
                Iterator<Unit> it = units.snapshotIterator();

                while (it.hasNext()) {

                    Unit u = it.next();
                    Stmt s = (Stmt) u;
                    if (s.containsFieldRef()) {
                        SootField ref = s.getFieldRef().getField();
                        SootField newref = oldFieldToNew.get(ref);

                        if (containsUse(u, ref)) {
                            System.out.println("-------------------------------");
                            System.out.println("found a use of: " + ref);
                            System.out.println("in stmt: " + s);
                            System.out.println("-------------------------------");
                        } else {
                            System.out.println("-------------------------------");
                            System.out.println("must have been a def of: " + ref);
                            System.out.println("in stmt: " + s);
                            System.out.println("-------------------------------");
                        }

                        if (newref != null) {
                            ValueBox fieldref = s.getFieldRefBox();
                            if (u.getUseBoxes().contains(fieldref)) {

                                SootMethod newAccessor = fieldToGetter.get(ref);
                                System.out.println("doing a field ref replace: " + ref + " --->" + newAccessor);
                                System.out.println("in this statement: " + s);
                                Local tmpRef = Jimple.v().newLocal("tmpRef", ref.getType());
                                body.getLocals().add(tmpRef);

                                if (addedMethod) {
                                    // the field ref is in a moved method
                                    if (ref.isStatic()) {
                                        System.out.println("building a static getter ref in added method");
                                        units.insertBefore(Jimple.v().newAssignStmt(tmpRef,
                                                Jimple.v().newStaticInvokeExpr(newAccessor.makeRef())), u);
                                    } else {
                                        System.out.println("building an instance getter ref in added method");
                                        units.insertBefore(Jimple.v().newAssignStmt(tmpRef, Jimple.v()
                                                .newVirtualInvokeExpr(body.getThisLocal(), newAccessor.makeRef())), u);
                                    }

                                } else {
                                    SootClass newClass = redefToNewClassMap.get(redefinition);

                                    // the field ref is in the redefinition class still, or some other class
                                    if (ref.isStatic()) {
                                        System.out.println(
                                                "building a static direct field (USE) access in non added method");
                                        units.insertBefore(Jimple.v().newAssignStmt(tmpRef, Jimple.v()
                                                .newStaticFieldRef(newClass.getFieldByName(ref.getName()).makeRef())),
                                                u);

                                    } else {
                                        System.out.println(
                                                "building a an instance field (USE) access in a non added method");
                                        Local newClassRef = lookup(newClass, newClass, body, units, u,
                                                ((InstanceFieldRef) s.getFieldRef()).getBase(), ogToHostHashTableName);
                                        units.insertBefore(
                                                Jimple.v().newAssignStmt(tmpRef,
                                                        Jimple.v().newInstanceFieldRef(newClassRef, newref.makeRef())),
                                                u);
                                    }
                                }
                                System.out.println(
                                        "This is whats in the box at the moment: " + s.getFieldRefBox().getValue());
                                s.getFieldRefBox().setValue(tmpRef);

                            }

                            if (u.getDefBoxes().contains(fieldref)) {
                                SootMethod newAccessor = fieldToSetter.get(ref);
                                System.out.println("doing a field ref replace: " + ref + " --->" + newAccessor);
                                System.out.println("in this statement: " + s);
                                if (addedMethod) {

                                    // def boxes only to be nonempty on identitystmts or assignstmts
                                    if (ref.isStatic()) {
                                        System.out.println("building a static setter ref in added method");
                                        units.insertBefore(
                                                Jimple.v().newInvokeStmt(Jimple.v()
                                                        .newStaticInvokeExpr(newAccessor.makeRef(), Arrays.asList(
                                                                new Value[] { ((DefinitionStmt) s).getRightOp() }))),
                                                u);
                                    } else {
                                        System.out.println("building an instance setter ref in added method");
                                        units.insertBefore(
                                                Jimple.v().newInvokeStmt(Jimple.v().newVirtualInvokeExpr(
                                                        (Local) ((InstanceFieldRef) s.getFieldRef()).getBase(),
                                                        newAccessor.makeRef(),
                                                        Arrays.asList(
                                                                new Value[] { ((DefinitionStmt) s).getRightOp() }))),
                                                u);
                                    }
                                } else {

                                    // the field ref is in the redefinition class still, or some other class
                                    if (ref.isStatic()) {
                                        System.out.println(
                                                "building an static field setter access in a non added meethod");
                                        units.insertBefore(
                                                Jimple.v().newInvokeStmt(Jimple.v()
                                                        .newStaticInvokeExpr(newAccessor.makeRef(), Arrays.asList(
                                                                new Value[] { ((DefinitionStmt) s).getRightOp() }))),
                                                u);

                                    } else {
                                        System.out.println(
                                                "building an instance field (DEF) access in a non added meethod");
                                        SootClass newClass = redefToNewClassMap.get(redefinition);
                                        Local newClassRef = lookup(newClass, newClass, body, units, u,
                                                ((InstanceFieldRef) s.getFieldRef()).getBase(), ogToHostHashTableName);
                                        units.insertBefore(
                                                Jimple.v().newInvokeStmt(Jimple.v().newVirtualInvokeExpr(newClassRef,
                                                        newAccessor.makeRef(),
                                                        Arrays.asList(
                                                                new Value[] { ((DefinitionStmt) s).getRightOp() }))),
                                                u);
                                    }
                                }
                                System.out.println("Removing statement: " + u);
                                units.remove(u);

                            }
                        }
                    }
                }
            }
        }
    }

    private static List<Unit> forwardPass(List<Local> relevantLocals, List<Unit> relevantStmts,
            PatchingChain<Unit> units) {
        List<Unit> changeSet = new ArrayList<Unit>();

        Unit pointer = units.getFirst();
        Unit last = units.getLast();
        while (pointer != last) {
            if (!relevantStmts.contains(pointer)) {
                List<Local> inStmt = extractLocals(pointer);
                List<Local> intersection = new ArrayList<Local>();
                intersection.addAll(inStmt);
                intersection.retainAll(relevantLocals);
                System.out.println("(FORWARD) intersection : " + intersection);
                if (intersection.size() != 0) {
                    System.out.println("(FORWARD) found relevant statement: " + pointer);
                    relevantStmts.add(pointer);
                    changeSet.add(pointer);

                    for (Local l : inStmt) {
                        System.out.println("(FORWARD) found this local: " + l);
                        relevantLocals.add(l);

                    }
                }
            }
            pointer = units.getSuccOf(pointer);
        }
        return changeSet;
    }

    private static List<Local> extractLocals(Unit u) {
        List<Local> locals = new ArrayList<Local>();
        for (ValueBox valuebox : u.getUseAndDefBoxes()) {
            Value value = valuebox.getValue();
            System.out.println("Looking at this valuebox: " + valuebox + "in this statment: " + u
                    + " with this getvalue: " + value);
            if (value instanceof Local && !locals.contains((Local) value)) {
                locals.add((Local) value);
            } else if (value instanceof InvokeExpr) {
                if (value instanceof InstanceInvokeExpr) {
                    // could it not be a local somehow?
                    if (((InstanceInvokeExpr) value).getBase() instanceof Local
                            && !locals.contains((Local) ((InstanceInvokeExpr) value).getBase())) {
                        locals.add((Local) ((InstanceInvokeExpr) value).getBase());
                    }
                }
                List<Value> invokeArgs = ((InvokeExpr) value).getArgs();
                for (Value arg : invokeArgs) {
                    if (arg instanceof Local && !locals.contains((Local) arg)) {
                        locals.add((Local) arg);
                    }
                }
            }
        }
        System.out.println("Found these locals: " + locals);
        return locals;
    }

    public static void createInitializer(SootClass newClass) {
        // only want to create one initialization function
        if (newClass.getMethodUnsafe("<init>", Arrays.asList(new Type[] {}), VoidType.v()) == null) {
            // using protected, but perhaps some other use cases need different access
            SootMethod initializer = new SootMethod("<init>", Arrays.asList(new Type[] {}), VoidType.v(),
                    Modifier.PROTECTED);
            JimpleBody body = Jimple.v().newBody(initializer);
            initializer.setActiveBody(body);
            Chain units = body.getUnits();
            Local selfref = Jimple.v().newLocal("selfref", newClass.getType());
            body.getLocals().add(selfref);
            units.add(Jimple.v().newIdentityStmt(selfref, new ThisRef(newClass.getType())));
            SootMethod parentConstructor = newClass.getSuperclass().getMethodUnsafe("<init>",
                    Arrays.asList(new Type[] {}), VoidType.v());
            if (parentConstructor == null) {
                System.out.println("The parent constructor was null...");
                // this cannot happen... the newClass is not Object... should throw here maybe
            }
            units.add(Jimple.v().newInvokeStmt(Jimple.v().newSpecialInvokeExpr(selfref, parentConstructor.makeRef())));
            units.add(Jimple.v().newReturnVoidStmt());
            newClass.addMethod(initializer);
        }
    }

    private static void createStaticInitializer(SootClass newClass) {
        // only want to create one static initialization function
        if (newClass.getMethodUnsafe("<clinit>", Arrays.asList(new Type[] {}), VoidType.v()) == null) {
            SootMethod initializer = new SootMethod("<clinit>", Arrays.asList(new Type[] {}), VoidType.v(),
                    Modifier.STATIC);
            JimpleBody body = Jimple.v().newBody(initializer);
            initializer.setActiveBody(body);
            Chain units = body.getUnits();
            units.add(Jimple.v().newReturnVoidStmt());
            newClass.addMethod(initializer);
        }
    }

    private boolean containsUse(Unit u, SootField ref) {
        for (ValueBox value : u.getUseBoxes()) {
            if (value.getValue().equivHashCode() == ref.equivHashCode()) {
                return true;
            }
        }
        return false;
    }

    /*
     * builds two maps hostToOriginal object refs originalToHost object refs not
     * efficient, but we need both ways
     */
    public static void buildHostMaps(SootClass newClass, String fieldname) {

        createStaticInitializer(newClass);

        /*
         * same beginning: java.util.Hashtable $r0; $r0 = new java.util.Hashtable;
         * specialinvoke $r0.<java.util.Hashtable: void <init>()>(); <Hashtableexample:
         * java.util.Hashtable xfieldtable> = $r0;
         */

        // add the field
        RefType HT = RefType.v("java.util.Hashtable");
        SootField replacementField = new SootField(fieldname + "HashTable", HT, Modifier.PUBLIC | Modifier.STATIC);
        newClass.addField(replacementField);
        // initialize it bc its static
        Body clinitBody = newClass.getMethod("<clinit>", Arrays.asList(new Type[] {}), VoidType.v())
                .retrieveActiveBody();
        PatchingChain<Unit> clinitUnits = clinitBody.getUnits();

        Local hashtable = Jimple.v().newLocal("HTTemp", HT);
        clinitBody.getLocals().add(hashtable);

        // build in bottom up order, bc we dont have any other reference point in the
        // method atm
        clinitUnits.addFirst(
                Jimple.v().newAssignStmt(Jimple.v().newStaticFieldRef(replacementField.makeRef()), hashtable));
        SootMethod toCall = Scene.v().getMethod("<java.util.Hashtable: void <init>()>");
        clinitUnits.addFirst(Jimple.v().newInvokeStmt(Jimple.v().newSpecialInvokeExpr(hashtable, toCall.makeRef())));
        clinitUnits.addFirst(Jimple.v().newAssignStmt(hashtable, Jimple.v().newNewExpr(HT)));
    }

    // we apply the same setup to all inits for the redef class
    public static void setupRedefInit(SootClass newClass, SootClass redef) {
        List<SootMethod> allMethods = redef.getMethods();
        for (SootMethod m : allMethods) {
            if (m.getName().equals("<init>")) {
                boolean buildHere = true;
                Body body = m.retrieveActiveBody();
                PatchingChain<Unit> units = body.getUnits();
                RefType HT = RefType.v("java.util.Hashtable");
                Local hashtableOGToHost = Jimple.v().newLocal("hashtableOGToHost", HT);
                body.getLocals().add(hashtableOGToHost);
                Local hashtableHostToOG = Jimple.v().newLocal("hashtableHostToOG", HT);
                body.getLocals().add(hashtableHostToOG);
                Local newclassref = Jimple.v().newLocal("newclassref", redef.getType());
                body.getLocals().add(newclassref);

                Local thisref = body.getThisLocal();
                Unit thisUnit = body.getThisUnit();
                Unit placementPoint = thisUnit;

                // find the super init in this init
                Iterator<Unit> it = units.snapshotIterator();
                while (it.hasNext()) {
                    Unit u = it.next();
                    Stmt s = (Stmt) u;
                    if (s.containsInvokeExpr() && s.getInvokeExpr() instanceof SpecialInvokeExpr
                            && ((SpecialInvokeExpr) s.getInvokeExpr()).getBase().equals(thisref)
                            && s.getInvokeExpr().getMethodRef().getName().contains("<init>")) {
                        placementPoint = u;
                        // actually dont want to build setup in init that calls other self initializer
                        // though
                        if (s.getInvokeExpr().getMethodRef().getDeclaringClass().getName().equals(redef.getName())) {
                            buildHere = false;
                        }
                        break;
                    }
                }
                if (buildHere) {
                    // just one more, since this is the insn AFTER super.init, and we're placing our
                    // hashtable inits on top of this ref
                    placementPoint = units.getSuccOf(placementPoint);

                    SootMethodRef hashtablePut = Scene.v()
                            .getMethod("<java.util.Hashtable: java.lang.Object put(java.lang.Object,java.lang.Object)>")
                            .makeRef();
                    units.insertBefore(Jimple.v().newAssignStmt(newclassref, Jimple.v().newNewExpr(newClass.getType())),
                            placementPoint);
                    units.insertBefore(
                            Jimple.v().newInvokeStmt(Jimple.v().newSpecialInvokeExpr(newclassref, newClass
                                    .getMethod("<init>", Arrays.asList(new Type[] {}), VoidType.v()).makeRef())),
                            placementPoint);

                    units.insertBefore(
                            Jimple.v()
                                    .newAssignStmt(hashtableOGToHost,
                                            Jimple.v().newStaticFieldRef(
                                                    newClass.getFieldByName(ogToHostHashTableName).makeRef())),
                            placementPoint);
                    units.insertBefore(Jimple.v().newInvokeStmt(Jimple.v().newVirtualInvokeExpr(hashtableOGToHost,
                            hashtablePut, Arrays.asList(new Value[] { thisref, newclassref }))), placementPoint);

                    units.insertBefore(
                            Jimple.v()
                                    .newAssignStmt(hashtableHostToOG,
                                            Jimple.v().newStaticFieldRef(
                                                    newClass.getFieldByName(hostToOgHashTableName).makeRef())),
                            placementPoint);
                    units.insertBefore(Jimple.v().newInvokeStmt(Jimple.v().newVirtualInvokeExpr(hashtableHostToOG,
                            hashtablePut, Arrays.asList(new Value[] { newclassref, thisref }))), placementPoint);
                }
            }
        }
    }

    /*
     * Builds a lookup of a object referring to some redefinition class mapped to
     * its host or vise versa
     */
    private Local lookup(SootClass newClass, SootClass classToMakeRefOf, Body body, PatchingChain<Unit> units,
            Unit insertBeforeHere, Value baseOfOriginalStmt, String whichTableToUse) {

        System.out.println("Building a hashtable lookup, table: " + whichTableToUse + " using this key for the map: "
                + baseOfOriginalStmt + " and adding it before this statement: " + insertBeforeHere);

        Local classToMakeRefOfRef = Jimple.v().newLocal("newClassRef", classToMakeRefOf.getType());
        body.getLocals().add(classToMakeRefOfRef);
        Local objecttemp = Jimple.v().newLocal("objecttemp", RefType.v("java.lang.Object"));
        body.getLocals().add(objecttemp);
        Local HT = Jimple.v().newLocal("HTFieldTemp", RefType.v("java.util.Hashtable"));
        body.getLocals().add(HT);

        units.insertBefore(
                Jimple.v().newAssignStmt(HT,
                        Jimple.v().newStaticFieldRef(newClass.getFieldByName(whichTableToUse).makeRef())),
                insertBeforeHere);

        SootMethodRef mapGetter = Scene.v().getMethod("<java.util.Hashtable: java.lang.Object get(java.lang.Object)>")
                .makeRef();
        units.insertBefore(Jimple.v().newAssignStmt(objecttemp,
                Jimple.v().newVirtualInvokeExpr(HT, mapGetter, Arrays.asList(new Value[] { baseOfOriginalStmt }))),
                insertBeforeHere);

        units.insertBefore(Jimple.v().newAssignStmt(classToMakeRefOfRef,
                Jimple.v().newCastExpr(objecttemp, classToMakeRefOf.getType())), insertBeforeHere);

        return classToMakeRefOfRef;

    }

    private void fixFieldRefsInAddedMethods(SootClass redefinition, List<SootMethod> addedMethods) {
        SootClass newClass = redefToNewClassMap.get(redefinition);

        for (SootMethod m : addedMethods) {
            Body body = m.retrieveActiveBody();
            PatchingChain<Unit> units = body.getUnits();
            Iterator<Unit> it = units.snapshotIterator();

            while (it.hasNext()) {

                Unit u = it.next();
                Stmt s = (Stmt) u;
                if (s.containsFieldRef()) {
                    SootField ref = s.getFieldRef().getField();
                    // the reference is for an instance field in the redefinition class and is not
                    // an added field
                    // shouldnt have to check this last one... but?
                    if (!ref.isStatic() && (redefinition.getFieldUnsafe(ref.getName(), ref.getType()) != null)
                            && (newClass.getFieldUnsafe(ref.getName(), ref.getType()) == null)) {
                        System.out.println("doing a field ref replace: " + ref + " --->");
                        System.out.println("in fixFieldRefsInAddedMethods, in this statement: " + s);

                        Local redefinitionClassRef = lookup(newClass, redefinition, body, units, u,
                                ((InstanceFieldRef) s.getFieldRef()).getBase(), hostToOgHashTableName);

                        System.out.println("This is wahts in the box atm: " + s.getFieldRefBox().getValue());
                        s.getFieldRefBox()
                                .setValue(Jimple.v().newInstanceFieldRef(redefinitionClassRef, ref.makeRef()));
                    }
                }
            }
        }
    }

    /*
     * fixes instance self ref'd method accesses in added methods, bc they will have
     * been pointing to this which will need to be translated using the map
     */
    public void fixMethodRefsInAddedMethods(SootClass redefinition, List<SootMethod> addedMethods) {
        System.out.println("------------------------------------------------");
        CallGraph cg = Scene.v().getCallGraph();
        List<SootMethod> redefMethods = redefinition.getMethods();
        for (SootMethod m : addedMethods) {

            Body body = m.retrieveActiveBody();
            PatchingChain<Unit> units = body.getUnits();
            Iterator<Unit> it = units.snapshotIterator();

            while (it.hasNext()) {

                Unit u = it.next();
                Stmt s = (Stmt) u;

                if (s.containsInvokeExpr()) {
                    InvokeExpr invokeExpr = s.getInvokeExpr();
                    if (invokeExpr instanceof InstanceInvokeExpr) {
                        System.out.println("Looking at this instance of invoke expr, in an added method: " + s);
                        Iterator targets = new Targets(explicitInvokesFilter.wrap(cg.edgesOutOf(s)));
                        while (targets.hasNext()) {
                            SootMethod next = (SootMethod) targets.next();
                            if (redefMethods.contains(next)) {
                                System.out.println(
                                        "Patching a target in an added method: " + next + " at this statement: " + s);
                                SootClass newClass = redefToNewClassMap.get(redefinition);
                                Local redefintionClassRef = lookup(newClass, redefinition, body, units, u,
                                        ((InstanceInvokeExpr) invokeExpr).getBase(), hostToOgHashTableName);
                                if (invokeExpr instanceof SpecialInvokeExpr) {
                                    // cannot special invoke even if we make public
                                    ValueBox invokebox = s.getInvokeExprBox();
                                    System.out
                                            .println("This is whats in the box atm: " + s.getFieldRefBox().getValue());
                                    invokebox.setValue(Jimple.v().newVirtualInvokeExpr(redefintionClassRef,
                                            next.makeRef(), invokeExpr.getArgs()));
                                } else {
                                    ((InstanceInvokeExpr) invokeExpr).setBase(redefintionClassRef);
                                }
                                break;
                            }
                        }
                    }
                    System.out.println("------------------------------------------------");
                }
            }
        }
    }

    /*
     * Replaces a method body with a call to the super method only for the case
     * where a patch removes a method and we want to preserve the intention of the
     * patch, but rm'ing the method made it redefinition non-compliant in this case
     * the removal of a method means calls that would have resolved to that removed
     * method now resolve to the parent
     *
     * unless there is no parent and refs are assumed to be rm'd in which case, it
     * is safe to simply re-add the method body, no harm
     */
    public static void fixRemovedMethods(List<SootMethod> methods) {
        for (SootMethod m : methods) {

            SootMethodRef parentMethodRef = findNearestImplementingParent(m);

            System.out.println("Fixing method refs in a removed method: " + m.getName() + " " + m.getDeclaringClass());
            System.out.println("Using this parent method ref as replacement: " + parentMethodRef.getName() + " "
                    + parentMethodRef.getDeclaringClass());

            if (parentMethodRef != null) {

                JimpleBody newBody = Jimple.v().newBody(m);
                Chain units = newBody.getUnits();

                Local selfref = Jimple.v().newLocal("selfref", m.getDeclaringClass().getType());
                newBody.getLocals().add(selfref);
                units.addFirst(Jimple.v().newIdentityStmt(selfref, new ThisRef(m.getDeclaringClass().getType())));

                List<Local> args = new ArrayList<Local>();

                for (int i = 0; i < m.getParameterCount(); i++) {
                    Local paramref = Jimple.v().newLocal("paramref", m.getParameterType(i));
                    newBody.getLocals().add(paramref);
                    units.add(
                            Jimple.v().newIdentityStmt(paramref, Jimple.v().newParameterRef(m.getParameterType(i), i)));
                    args.add(paramref);
                }

                if (m.getReturnType() instanceof VoidType) {
                    units.add(
                            Jimple.v().newInvokeStmt(Jimple.v().newSpecialInvokeExpr(selfref, parentMethodRef, args)));
                    units.add(Jimple.v().newReturnVoidStmt());
                } else {
                    Local ret = Jimple.v().newLocal("retValue", m.getReturnType());
                    newBody.getLocals().add(ret);
                    units.add(Jimple.v().newAssignStmt(ret,
                            Jimple.v().newSpecialInvokeExpr(selfref, parentMethodRef, args)));
                    units.add(Jimple.v().newReturnStmt(ret));
                }

                m.setActiveBody(newBody);
            }
        }
    }

    private static SootMethodRef findNearestImplementingParent(SootMethod m) {
        SootClass obj = Scene.v().getSootClass("java.lang.Object");
        if (m.getDeclaringClass() == obj) {
            return null;
        }
        SootClass parent = m.getDeclaringClass().getSuperclass();
        while (parent != null) {
            SootMethod parentMethod = parent.getMethodUnsafe(m.getName(), m.getParameterTypes(), m.getReturnType());
            if (parentMethod != null) {
                SootMethodRef parentMethodRef = parentMethod.makeRef();
                return parentMethodRef;
            } else {
                parent = parent.getSuperclassUnsafe();
            }
        }
        return null;
    }
}
