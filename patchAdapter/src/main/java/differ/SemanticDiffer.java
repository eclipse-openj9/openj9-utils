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

import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.io.IOException;

import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.nio.file.Paths;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.ParseException;

import soot.G;
import soot.Main;
import soot.PackManager;
import soot.Scene;
import soot.SourceLocator;
import soot.Transform;
import soot.Transformer;
import soot.SceneTransformer;
import soot.SootClass;
import soot.SootField;
import soot.SootMethod;
import soot.jimple.JasminClass;
import soot.options.Options;
import soot.util.JasminOutputStream;

public class SemanticDiffer {

    private static CommandLine options;
    private static String renameResultDir = ".";
    private static String originalRenameSuffix = "Original";
    private static HashMap<SootClass, SootClass> newClassMap = new HashMap<SootClass, SootClass>();
    private static HashMap<SootClass, SootClass> newClassMapReversed = new HashMap<SootClass, SootClass>();
    private static PatchTransformer patchTransformer;
    private static HashMap<SootClass, SootClass> originalToRedefinitionClassMap = new HashMap<SootClass, SootClass>();
    private static List<String> allEmittedClassesRedefs = new ArrayList<String>();
    private static List<String> allEmittedClassesHostClasses = new ArrayList<String>();
    private static List<String> originalClassesList = new ArrayList<String>();
    private static List<String> loadedRenamedClasses = new ArrayList<String>();
    private static boolean doDiff = true;

    private static HashMap<SootClass, List<CheckSummary>> redefToDiffSummary = new HashMap<SootClass, List<CheckSummary>>();

    private static ArrayList<String> allOGNames = new ArrayList<String>();
    private static ArrayList<String> allOGNamesRenamed = new ArrayList<String>();
    private static HashMap<SootClass, ArrayList<SootMethod>> allRemovedMethods = new HashMap<SootClass, ArrayList<SootMethod>>();

    public static void main(String[] args) throws ParseException {

        // reset if previously run, in same JVM
        allOGNames.clear();
        allOGNamesRenamed.clear();
        originalToRedefinitionClassMap.clear();

        // doesnt die on unknown options, will pass them to soot
        RelaxedParser parser = new RelaxedParser();
        options = parser.parse(new SemanticOptions(), args);
        List<String> options1 = new ArrayList<String>();
        List<String> options2 = new ArrayList<String>();
        List<String> options1final = new ArrayList<String>();
        List<String> options2final = new ArrayList<String>();

        if (options.hasOption("finalDestination") && !options.hasOption("renameDestination")) {
            System.out.println("Using output dir for both soot runs: " + options.getOptionValue("finalDestination"));
            parser.getLeftovers(options2);
            options1.add("-d");
            options1.add(options.getOptionValue("finalDestination"));
            renameResultDir = options.getOptionValue("finalDestination");
        } else if (!options.hasOption("finalDestination") && options.hasOption("renameDestination")) {
            System.out.println("Using output dir for both soot runs: " + options.getOptionValue("renameDestination"));
            parser.getLeftovers(options1);
            options1.add("-d");
            options1.add(options.getOptionValue("renameDestination"));
            renameResultDir = options.getOptionValue("renameDestination");
        } else if (options.hasOption("finalDestination") && options.hasOption("renameDestination")) {
            parser.getLeftovers(options1);
            options1.add("-d");
            options1.add(options.getOptionValue("renameDestination"));
            renameResultDir = options.getOptionValue("renameDestination");
            parser.getLeftovers(options2);
            options2.add("-d");
            options2.add(options.getOptionValue("finalDestination"));
        }

        if (options.hasOption("runRename") && options.getOptionValue("runRename").equals("true")) {
            PackManager.v().getPack("wjtp").add(new Transform("wjtp.renameTransform", createRenameTransformer()));
            System.err.println("----------------------------------------------");
            System.out.println(
                    "SSDIFF: these were the first two options: " + options1.get(0) + " AND " + options1.get(1));
            // trust that if differ is already running with cp options, this run can share
            // those, otherwise soot will fail. if not the case, run sequential runs.
            if (Options.v().soot_classpath().isEmpty()) {
                for (int i = 0; i < options1.size(); i++) {
                    options1final.add(options1.get(i));
                }
            } else {
                for (int i = 2; i < options1.size(); i++) {
                    options1final.add(options1.get(i));
                }
            }
            if (!Options.v().output_dir().isEmpty()) {
                options1final.remove("-d");
                options1final.remove(options1final.size() - 1);
            }

            System.out.println("First soot has these options: " + options1final);
            System.err.println("----------------------------------------------");
            soot.Main.main(options1final.toArray(new String[0]));
            // not sure if this is needed, or if it is included in the overall reset
            PackManager.v().getPack("wjtp").remove("wjtp.renameTransform");
            G.reset();
        }
        if (doDiff) {
            // means that there was something to rename
            PackManager.v().getPack("wjtp").add(new Transform("wjtp.myTransform", createDiffTransformer()));
            options2final.add("-w");

            // could handle the mainClass setting better, if useFullDir was false
            if (options.hasOption("useFullDir") && options.getOptionValue("useFullDir").equals("true")) {
                options2final.add("-process-dir");
                options2final.add(options.getOptionValue("redefcp"));
            } else {
                if (loadedRenamedClasses.size() == 1
                        && !loadedRenamedClasses.contains(options.getOptionValue("mainClass"))) {
                    options2final.add(loadedRenamedClasses.get(0));
                } else {
                    options2final.add(options.getOptionValue("mainClass"));
                }
            }

            Options.v().set_soot_classpath(options.getOptionValue("redefcp") + ":" + options1.get(1));

            Options.v().set_output_dir(options.getOptionValue("finalDestination"));
            Options.v().set_allow_phantom_refs(true);
            Options.v().setPhaseOption("cg", "all-reachable:true");

            System.out.println("Second soot has these options: " + options2final);
            System.out.println("Second soot has this classpath: " + Scene.v().getSootClassPath());
            System.err.println("----------------------------------------------");
            soot.Main.main(options2final.toArray(new String[0]));
        }
    }

    private static Transformer createRenameTransformer() {
        return new SceneTransformer() {
            protected void internalTransform(String phaseName, Map optionsFromTransform) {
                if (options.hasOption("originalclasslist")) {
                    System.out.println("This is the classlist file to use for reading original classes to rename: "
                        + options.getOptionValue("originalclasslist"));
                    ArrayList<SootClass> allOG = gatherClassesFromFile(options.getOptionValue("originalclasslist"));

                    Scene.v().getApplicationClasses().clear();
                    for (SootClass original : allOG) {
                        original.rename(original.getName() + originalRenameSuffix);
                        Scene.v().getOrAddRefType(original.getType());
                        original.setApplicationClass();
                    }
                    System.err.println("Finished rename phase.");
                    System.err.println("----------------------------------------------");
                } else {
                    System.err.println("Did not receive a list of classes to rename.");
                    doDiff = false;
                }
            }
        };
    }

    private static Transformer createDiffTransformer() {
        return new SceneTransformer() {
            protected void internalTransform(String phaseName, Map originalOptions) {

                List<String> mainClassPackageNameComponents = new ArrayList<String>();
                String[] mainClassPackageNameComponentsAll;
                // TODO ideally have better way to know package names of many classes in patch
                if (loadedRenamedClasses.size() == 1
                        && !loadedRenamedClasses.contains(options.getOptionValue("mainClass"))) {
                    mainClassPackageNameComponentsAll = loadedRenamedClasses.get(0).split("\\.");
                } else {
                    mainClassPackageNameComponentsAll = options.getOptionValue("mainClass").split("\\.");
                }

                System.out.println(Arrays.toString(mainClassPackageNameComponentsAll));
                for (int i = 0; i < mainClassPackageNameComponentsAll.length - 1; i++) {
                    mainClassPackageNameComponents.add(mainClassPackageNameComponentsAll[i]);
                }
                String mainClassPackageName = String.join("/", mainClassPackageNameComponents) + "/";
                Scene.v().getApplicationClasses().clear();

                System.out.println("-------------------------------");
                Scene.v().setSootClassPath(options.getOptionValue("redefcp") + ":" + Scene.v().getSootClassPath());
                System.err.println("Classpath before redef/patch classes load: " + Scene.v().getSootClassPath());
                ArrayList<SootClass> allRedefs = gatherClassesFromDir(
                        options.getOptionValue("redefcp") + "/" + mainClassPackageName,
                        mainClassPackageName.replace("/", "."), allOGNames);
                for (SootClass redef : allRedefs) {
                    // this is so that the redef classes will also get output'd by soot
                    redef.setApplicationClass();
                }

                System.err.println("Classes after redef load: ");
                System.err.println(Scene.v().getApplicationClasses());

                System.out.println("-------------------------------");
                Scene.v().setSootClassPath(renameResultDir + ":" + Scene.v().getSootClassPath());
                System.err.println("Classpath before renamed/original classes load: " + Scene.v().getSootClassPath());
                ArrayList<SootClass> allOriginals = gatherClassesFromDir(renameResultDir + "/" + mainClassPackageName,
                        mainClassPackageName.replace("/", "."), allOGNamesRenamed);

                sortClasses(allOriginals, allRedefs);
                System.err.println("Classes map after the sort: " + originalToRedefinitionClassMap);

                System.err.println("Resulting application classes after at beginning of diff phase: ");
                System.err.println(Scene.v().getApplicationClasses());

                patchTransformer = new PatchTransformer(newClassMap, newClassMapReversed);

                // TODO find better way to init, some of this wont work under nonequal og:redef
                // ratios
                System.out.println("-------------------------------");
                for (SootClass redef : allRedefs) {
                    System.out.println("Creating a new class with the following name: " + redef.getName() + "NewClass");
                    SootClass newClass = new SootClass(redef.getName() + "NewClass", redef.getModifiers());
                    newClassMap.put(redef, newClass);
                    newClassMapReversed.put(newClass, redef);
                }
                System.out.println("-------------------------------");

                for (SootClass redef : allRedefs) {
                    SootClass newClass = newClassMap.get(redef);
                    // if there is a host for the super, set that as its host's super
                    if (newClassMap.get(redef.getSuperclass()) != null) {
                        System.out.println("New class map not null - Setting the newclass's super, new is: "
                                + newClass.getName() + "super is: " + newClassMap.get(redef.getSuperclass()));
                        newClass.setSuperclass(newClassMap.get(redef.getSuperclass()));
                    } else {
                        System.out.println("Setting the newclass's super, new is: " + newClass.getName() + "super is: "
                                + redef.getSuperclass().getName());
                        newClass.setSuperclass(redef.getSuperclass());
                    }

                    PatchTransformer.createInitializer(newClass);
                    // all newclasses will have maps to track who they belong with
                    PatchTransformer.buildHostMaps(newClass, "originalToHost");
                    PatchTransformer.buildHostMaps(newClass, "hostToOriginal");
                    PatchTransformer.setupRedefInit(newClass, redef);
                }

                for (SootClass og : originalToRedefinitionClassMap.keySet()) {
                    diff(og, originalToRedefinitionClassMap.get(og));
                }
                // now fix all of the method references everywhere, in classes we are outputting
                for (SootClass redef : allRedefs) {

                    if (allRemovedMethods.get(redef) != null) {
                        System.out.println("-------------------------------");
                        System.out.println("Fixing references in removed method: ");
                        System.out.println(redef.getName() + " " + allRemovedMethods.get(redef));
                        patchTransformer.fixRemovedMethods(allRemovedMethods.get(redef));
                        System.out.println("-------------------------------");
                    }

                    patchTransformer.transformMethodCalls(redef.getMethods(), true);
                    // might need to omit the init and clinit on this one?
                    patchTransformer.transformMethodCalls(newClassMap.get(redef).getMethods(), false);
                }

                // weird hack to get soot/asm to fix all references in the class to align with
                // renaming to OG name
                for (SootClass redef : allRedefs) {
                    String ogRedefName = redef.getName();
                    redef.rename(ogRedefName + "Original");
                    redef.rename(ogRedefName);
                    allEmittedClassesRedefs.add(redef.getName());
                }
                System.out.println("======================================================");
                for (SootClass newCls : newClassMap.values()) {
                    if (!Scene.v().containsClass(newCls.getName()) && !newCls.isInScene()) {
                        Scene.v().addClass(newCls);
                        allEmittedClassesHostClasses.add(newCls.getName());
                        writeNewClass(newCls);
                    }
                }
                System.out.println("======================================================");
            }
        };
    }

    // not currently responsible for validating that the patch contains at least the
    // classes in the original set
    private static void sortClasses(ArrayList<SootClass> allOriginals, ArrayList<SootClass> allRedefs) {
        for (SootClass redef : allRedefs) {
            // unfortunately not efficient since we're doing a name compare, but shouldnt be
            // a large set...
            for (SootClass og : allOriginals) {
                if ((redef.getName() + "Original").equals(og.getName())) {
                    originalToRedefinitionClassMap.put(og, redef);
                    break;
                }
            }
        }
    }

    // thank you tutorial: https://www.sable.mcgill.ca/soot/tutorial/createclass/
    private static void writeNewClass(SootClass newClass) {
        try {
            String fileName = SourceLocator.v().getFileNameFor(newClass, Options.output_format_class);
            System.out.println("Writing to file using this directory and filename: " + fileName);
            File file = new File(fileName);
            file.getParentFile().mkdirs();
            OutputStream streamOut = new JasminOutputStream(new FileOutputStream(file));
            PrintWriter writerOut = new PrintWriter(new OutputStreamWriter(streamOut));

            JasminClass jasminClass = new JasminClass(newClass);
            jasminClass.print(writerOut);
            writerOut.flush();
            streamOut.close();
        } catch (IOException e) {
            System.out.println("Could not write a transformer redefinition file!");
            e.printStackTrace();
        }

    }

    private static void diff(SootClass original, SootClass redefinition) {
        System.err.println("\n##########################################");
        System.err.println("Diff Report for original: " + original.getName() + " compared to redefinition: "
                + redefinition.getName());
        CheckSummary fieldSummary = checkFields(original, redefinition);
        CheckSummary methodSummary = checkMethods(original, redefinition);
        List<CheckSummary> summaryList = new ArrayList<CheckSummary>();
        summaryList.add(methodSummary);
        summaryList.add(fieldSummary);
        redefToDiffSummary.put(redefinition, summaryList);
        System.err.println("---------------------------------------");
        checkInheritance(original, redefinition);
        System.err.println("---------------------------------------");
        finishFieldSummary(original, fieldSummary, redefinition, methodSummary);
        System.err.println("---------------------------------------");
        finishMethodSummary(methodSummary, redefinition);
        System.err.println("---------------------------------------");
        System.err.println("##########################################\n");
    }

    private static CheckSummary checkFields(SootClass original, SootClass redefinition) {
        HashMap<SootField, SootField> originalToRedefinitionMap = new HashMap<SootField, SootField>();
        CheckSummary fieldSummary = new CheckSummary<SootField>();

        // to avoid recomputing the equivalence checks for all original fields
        HashMap<Integer, SootField> originalHashFields = new HashMap<Integer, SootField>();
        HashMap<Integer, SootField> redefinitionHashFields = new HashMap<Integer, SootField>();

        for (SootField field : original.getFields()) {
            originalHashFields.put(new Integer(field.equivHashCode()), field);
        }

        for (SootField field : redefinition.getFields()) {
            boolean matched = false;
            Integer hash = new Integer(field.equivHashCode());
            redefinitionHashFields.put(hash, field);
            if (!originalHashFields.containsKey(hash)) {
                // modified pre-existing field
                for (SootField originalField : original.getFields()) {

                    boolean sameType = originalField.getType().toString().equals(field.getType().toString());
                    boolean sameName = originalField.getName().equals(field.getName());
                    boolean sameModifiers = originalField.getModifiers() == field.getModifiers();

                    if (sameType && sameName) {
                        // modified modifiers
                        matched = true;
                        originalToRedefinitionMap.put(originalField, field);

                        System.err.println("\t The following field has had modifiers altered: \n");
                        System.err.println(originalField.getDeclaration() + "  --->  " + field.getDeclaration());
                    } else if (sameType && sameModifiers) {
                        matched = true;
                        originalToRedefinitionMap.put(originalField, field);

                        System.err.println("\t The following field has had its name altered: \n");
                        System.err.println(originalField.getDeclaration() + "  --->  " + field.getDeclaration());
                    } else if (sameModifiers && sameName) {
                        matched = true;
                        originalToRedefinitionMap.put(originalField, field);

                        System.err.println("\t The following field has had its type altered: \n");
                        System.err.println(originalField.getDeclaration() + "  --->  " + field.getDeclaration());

                    }

                }
                if (!matched) {
                    // added field means at least two (or even all three) of the above id factors
                    // are different
                    fieldSummary.addAddedItem(field);
                }
            }
        }
        for (SootField field : original.getFields()) {
            if (!originalToRedefinitionMap.containsKey(field)
                    && !redefinitionHashFields.containsKey(new Integer(field.equivHashCode()))) {
                fieldSummary.addRemovedItem(field);
            }
        }

        if (originalToRedefinitionMap.size() == 0) {
            fieldSummary.changes = false;
        } else {
            fieldSummary.changes = true;
        }
        return fieldSummary;

    }

    private static void finishFieldSummary(SootClass original, CheckSummary fieldSummary, SootClass redefinition,
            CheckSummary methodSummary) {

        patchTransformer.fixStaticFieldValueChanges(original, redefinition, fieldSummary.addedList);
        if (fieldSummary.addedList.size() != 0) {
            System.err.println("\t Field(s) have been added.");
            System.err.println(fieldSummary.addedList);
        } else if (fieldSummary.removedList.size() != 0) {
            System.err.println("\tField(s) has been removed");
            System.err.println(fieldSummary.removedList);
            for (Object generic : fieldSummary.removedList) {
                SootField f = (SootField) generic;
                f.setDeclared(false);
                redefinition.addField(f);
            }
        } else if (!fieldSummary.changes) {
            System.err.println("\tNo Field differences!");
        }
        // fix field refs in potentially added methods even if no fields added
        patchTransformer.transformFields(original, redefinition, fieldSummary.addedList, methodSummary.addedList);

    }

    private static CheckSummary checkMethods(SootClass original, SootClass redefinition) {
        HashMap<SootMethod, SootMethod> originalToRedefinitionMap = new HashMap<SootMethod, SootMethod>();
        CheckSummary methodSummary = new CheckSummary<SootMethod>();

        // to avoid recomputing the equivalence checks for all original methods
        HashMap<Integer, SootMethod> originalHashMethods = new HashMap<Integer, SootMethod>();
        HashMap<Integer, SootMethod> redefinitionHashMethods = new HashMap<Integer, SootMethod>();

        for (SootMethod method : original.getMethods()) {
            originalHashMethods.put(new Integer(ownEquivMethodHash(method)), method);
        }

        for (SootMethod method : redefinition.getMethods()) {

            boolean matched = false;
            Integer hash = new Integer(ownEquivMethodHash(method));
            redefinitionHashMethods.put(hash, method);
            if (!originalHashMethods.containsKey(hash)) {
                // modified pre-existing method
                for (SootMethod originalMethod : original.getMethods()) {

                    // if there is an exact match for this original item, dont bother to ask if it
                    // matches a transformed item
                    if (!redefinitionHashMethods.containsKey(ownEquivMethodHash(originalMethod))) {

                        boolean sameType = originalMethod.getReturnType().toString()
                                .equals(method.getReturnType().toString());
                        boolean sameName = originalMethod.getName().equals(method.getName());
                        boolean sameModifiers = originalMethod.getModifiers() == method.getModifiers();
                        boolean sameParameters = originalMethod.getParameterTypes().equals(method.getParameterTypes());

                        if (sameType && sameName && sameParameters && !sameModifiers) {
                            // modified modifiers
                            matched = true;
                            originalToRedefinitionMap.put(originalMethod, method);

                            System.err.println("\t The following method has had modifiers altered: \n");
                            System.err.println(originalMethod.getDeclaration() + "  --->  " + method.getDeclaration());
                        }
                        /*
                         * else if(sameType && sameModifiers && sameParameters){ //modified name,
                         * actually not sure if we can even detect this truly, currently leaving here as
                         * //option, but is handled as new method, not altered method matched = true;
                         * originalToRedefinitionMap.put(originalMethod, method);
                         * 
                         * System.err.println("\t The following method has had its name altered: \n");
                         * System.err.println(originalMethod.getDeclaration() + "  --->  " +
                         * method.getDeclaration()); }
                         */
                        else if (sameModifiers && sameName && sameParameters && !sameType) {
                            // modified return type
                            matched = true;
                            originalToRedefinitionMap.put(originalMethod, method);

                            System.err.println("\t The following method has had its type altered: \n");
                            System.err.println(originalMethod.getDeclaration() + "  --->  " + method.getDeclaration());

                        }
                    }
                }
                if (!matched) {
                    // added method means at least two (or even all three) of the above id factors
                    // are different
                    methodSummary.addAddedItem(method);
                }
            }
        }
        for (SootMethod method : original.getMethods()) {
            if (!originalToRedefinitionMap.containsKey(method)
                    && !redefinitionHashMethods.containsKey(new Integer(ownEquivMethodHash(method)))) {
                methodSummary.addRemovedItem(method);
            }
        }

        if (originalToRedefinitionMap.size() == 0) {
            methodSummary.changes = false;
        } else {
            methodSummary.changes = true;
        }

        return methodSummary;
    }

    private static void finishMethodSummary(CheckSummary methodSummary, SootClass redefinition) {
        if (methodSummary.addedList.size() != 0) {
            System.err.println("\t Method(s) have been added.");
            System.err.println(methodSummary.addedList);
            // do the method moving as we go
            patchTransformer.moveMethodCalls(redefinition, methodSummary.addedList);
            // also fix the instance method refs that exist in moved methods, since "this"
            // is now wrong
            patchTransformer.fixMethodRefsInAddedMethods(redefinition, methodSummary.addedList);
        } else if (methodSummary.removedList.size() != 0) {
            allRemovedMethods.put(redefinition, new ArrayList<SootMethod>());
            System.err.println("\tMethod(s) has been removed");
            System.err.println(methodSummary.removedList);
            for (Object generic : methodSummary.removedList) {
                SootMethod m = (SootMethod) generic;
                m.setDeclared(false);
                redefinition.addMethod(m);
                allRemovedMethods.get(redefinition).add(m);
            }
        } else if (!methodSummary.changes) {
            System.err.println("\tNo Method differences!");
        }
    }

    private static void checkInheritance(SootClass original, SootClass redefinition) {
        boolean originalHasSuper = original.hasSuperclass();
        boolean redefinitionHasSuper = redefinition.hasSuperclass();
        if ((originalHasSuper && !redefinitionHasSuper) || (!originalHasSuper && redefinitionHasSuper)) {
            System.err.println("\tInheritance Diff!");
            System.err.println("\tOriginal class has superclass: " + original.getSuperclassUnsafe()
                    + " and redefinition has superclass: " + redefinition.getSuperclassUnsafe());

        } else if (redefinition.hasSuperclass() && original.hasSuperclass()
                && !(redefinition.getSuperclass().getName().equals(original.getSuperclass().getName()))
                && !(redefinition.getSuperclass().getName() + originalRenameSuffix)
                        .equals(original.getSuperclass().getName())) {
            System.err.println("\tInheritance Diff!");
            System.err.println("\tOriginal class has superclass: " + original.getSuperclassUnsafe()
                    + " and redefinition has superclass: " + redefinition.getSuperclassUnsafe());

        } else if (original.getInterfaceCount() != redefinition.getInterfaceCount()) {
            System.err.println("\tInheritance Diff!");
            System.err.println("\tOriginal class has interfaces: " + original.getInterfaces()
                    + " and redefinition has interfaces: " + redefinition.getInterfaces());
        } else {
            System.err.println("\tNo Inheritance differences!");
        }
    }

    protected static int ownEquivMethodHash(SootMethod method) {
        // considers the params, since they would be in a signature, unlike:
        // https://github.com/Sable/soot/blob/master/src/main/java/soot/SootMethod.java#L133
        // basically we treat each definition of an overloaded method as different,
        // while soot would not (for some reason)
        return method.getReturnType().hashCode() * 101 + method.getModifiers() * 17 + method.getName().hashCode()
                + method.getParameterTypes().hashCode();

    }

    // resolves all of the classes in some dir that defines the patch (== set of
    // classes)
    private static ArrayList<SootClass> gatherClassesFromDir(String strdir, String packagename,
            ArrayList<String> OGList) {
        System.out.println("Gathering classes from: " + strdir);
        ArrayList<String> allNames = new ArrayList<String>();
        try {
            File dir = new File(strdir);
            File[] directoryListing = dir.listFiles();
            if (directoryListing != null) {
                for (File file : directoryListing) {
                    if (file.toString().contains("class")) {
                        // ugly parsing, its the only way tho?
                        String classname = null;
                        if (packagename.equals(".")) {
                            // no package prefix
                            String[] namepieces = file.toString().replace(".class", "").split("/");
                            classname = namepieces[namepieces.length - 1];
                            if (OGList.contains(classname)) {
                                System.out.println("Gathering class: " + classname);
                                allNames.add(classname);
                            }
                        } else {
                            classname = file.toString().replaceFirst(strdir, "").replace(".class", "").replaceAll("\\/",
                                    ".");
                            if (OGList.contains(packagename + classname)) {
                                System.out.println("Gathering class: " + packagename + classname);
                                allNames.add(packagename + classname);
                            }
                        }
                    }
                }
            } else {
                System.out.println("Directory supplied is not sufficient to read.");
            }
        } catch (Exception e) {
            System.out.println("Some issue accessing the classes to be renamed: " + e.getMessage());
        }
        ArrayList<SootClass> allClasses = resolveClasses(allNames);
        return allClasses;
    }

    // reads the classes that designate the patch, from a file. One class per line,
    // fqn.
    // needs to exist bc each class to analyse may require many classes to be
    // patched for it
    private static ArrayList<SootClass> gatherClassesFromFile(String originalClassesList) {
        ArrayList<SootClass> allClasses = new ArrayList<SootClass>();
        try {
            if (new File(originalClassesList).exists()) {
                System.out.println("SSDIFF: Reading patch classes from: " + originalClassesList);
                BufferedReader in = new BufferedReader(new FileReader(originalClassesList));
                String str;
                while ((str = in.readLine()) != null) {
                    String name = str.replace(".class", "").replaceAll("\\/", ".");
                    if (!allOGNames.contains(name)) {
                        allOGNames.add(name);
                        allOGNamesRenamed.add(name + originalRenameSuffix);
                        loadedRenamedClasses.add(name);
                    }
                }
                allClasses = resolveClasses(allOGNames);
            }
        } catch (Exception e) {
            System.out.println("Some issue accessing the classes to be renamed: " + e.getMessage());
        }
        return allClasses;
    }

    private static ArrayList<SootClass> resolveClasses(ArrayList<String> allNames) {
        ArrayList<SootClass> allClasses = new ArrayList<SootClass>();
        for (String classname : allNames) {
            System.out.println("Resolving class: " + classname);
            SootClass resolvedClass = Scene.v().forceResolve(classname, SootClass.BODIES);
            allClasses.add(resolvedClass);
        }
        return allClasses;
    }

    public static List<String> getGeneratedClassesRedefs() {
        return allEmittedClassesRedefs;
    }

    public static List<String> getGeneratedClassesHosts() {
        return allEmittedClassesHostClasses;
    }

}
