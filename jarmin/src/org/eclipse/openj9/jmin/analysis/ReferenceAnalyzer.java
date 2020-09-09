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

import org.eclipse.openj9.JMin;
import org.eclipse.openj9.jmin.info.*;
import org.eclipse.openj9.jmin.util.Config;
import org.eclipse.openj9.jmin.util.HierarchyContext;
import org.objectweb.asm.tree.analysis.Analyzer;
import org.objectweb.asm.tree.analysis.AnalyzerException;
import org.objectweb.asm.tree.analysis.BasicValue;
import org.objectweb.asm.tree.analysis.Frame;

import org.objectweb.asm.AnnotationVisitor;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.Handle;
import org.objectweb.asm.Type;
import org.objectweb.asm.tree.AbstractInsnNode;
import org.objectweb.asm.tree.AnnotationNode;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.FieldInsnNode;
import org.objectweb.asm.tree.FieldNode;
import org.objectweb.asm.tree.InvokeDynamicInsnNode;
import org.objectweb.asm.tree.LdcInsnNode;
import org.objectweb.asm.tree.MethodInsnNode;
import org.objectweb.asm.tree.MethodNode;
import org.objectweb.asm.tree.TypeInsnNode;

import static org.objectweb.asm.Opcodes.*;
import static org.objectweb.asm.Type.ARRAY;
import static org.objectweb.asm.Type.OBJECT;

import java.util.List;

public class ReferenceAnalyzer {
    public static ClassVisitor getReferenceInfoProcessor(ClassSource source, ReferenceInfo info, HierarchyContext context) {
        return new ClassNode(ASM8) {
            private boolean isAnnotation;
            private String clazz;
            private ClassSource classSource;
            @Override
            public void visit(int version,
                  int access,
                  String name,
                  String signature,
                  String superName,
                  String[] interfaces) {
                this.clazz = name;
                this.classSource = source;
                isAnnotation = (access & ACC_ANNOTATION) != 0;
                context.addClassToJarMapping(clazz, source);
                context.addSuperClass(clazz, superName);
                context.addInterfaces(clazz, interfaces);
                super.visit(version, access, name, signature, superName, interfaces);
            }

            @Override
            public AnnotationVisitor visitAnnotation(final String descriptor, final boolean visible) {
                AnnotationVisitor inner = super.visitAnnotation(descriptor, visible);
                if (isAnnotation && descriptor.equals("Ljava/lang/annotation/Retention;")) {
                    return new AnnotationVisitor(ASM8, inner) {
                        @Override
                        public void visitEnum(final String name, final String descriptor,
                                final String value) {
                            if (av != null) {
                                av.visitEnum(name, descriptor, value);
                            }
                            if (name.equals("value") && value.equals("RUNTIME")) {
                                context.addRuntimeAnnotation(clazz);
                            }
                        }
                    };
                }
                return inner;
            }

            @Override
            public void visitEnd() {
                ClassInfo cinfo = info.addClass(clazz);
                if (visibleAnnotations != null) {
                    for (AnnotationNode an : visibleAnnotations) {
                        cinfo.addAnnotation(an.desc.substring(1, an.desc.length() - 1));
                    }
                }
                for (FieldNode fn : fields) {
                    cinfo.addField(fn.name, fn.desc);
                    if (fn.visibleAnnotations != null) {
                        for (AnnotationNode an : fn.visibleAnnotations) {
                            cinfo.getField(fn.name, fn.desc).addAnnotation(an.desc.substring(1, an.desc.length() -1));
                        }
                    }
                }
                for (MethodNode mn : (List<MethodNode>) methods) {
                    try {
                        processMethod(clazz, mn, info, context, classSource);
                    } catch (AnalyzerException e) {
                        System.out.println("AnalyzerException caught when processing method " + cinfo.name() + " " + mn.name + " " + mn.desc);
                    }
                    if (mn.visibleAnnotations != null) {
                        for (AnnotationNode an : mn.visibleAnnotations) {
                            cinfo.getMethod(mn.name, mn.desc).addAnnotation(an.desc.substring(1, an.desc.length() - 1));
                        }
                    }
                    if (mn.visibleParameterAnnotations != null) {
                        for (List<AnnotationNode> pan : mn.visibleParameterAnnotations) {
                            if (pan != null) {
                                for (AnnotationNode an : pan) {
                                    cinfo.getMethod(mn.name, mn.desc).addAnnotation(an.desc.substring(1, an.desc.length() -1));
                                }
                            }
                        }
                    }

                }
                super.visitEnd();
            }
        };
    }

    private static void processMethod(String owner, MethodNode mn, ReferenceInfo info, HierarchyContext context, ClassSource classSource) throws AnalyzerException {
        ClassInfo cinfo = info.getClassInfo(owner);
        MethodInfo minfo = cinfo.addMethod(mn.name, mn.desc);
        for (Type t : Type.getArgumentTypes(mn.desc)) {
            if (t.getSort() == Type.OBJECT) {
                minfo.addReferencedClass(t.getInternalName());
            } else if (t.getSort() == Type.ARRAY) {
                do {
                    t = t.getElementType();
                } while (t.getSort() == Type.ARRAY);
                if (t.getSort() == Type.OBJECT) {
                    minfo.addReferencedClass(t.getInternalName());
                }
            }
        }
        {
            Type t = Type.getReturnType(mn.desc);
            if (t.getSort() == Type.OBJECT) {
                minfo.addReferencedClass(t.getInternalName());
            } else if (t.getSort() == Type.ARRAY) {
                do {
                    t = t.getElementType();
                } while (t.getSort() == Type.ARRAY);
                if (t.getSort() == Type.OBJECT) {
                    minfo.addReferencedClass(t.getInternalName());
                }
            }
        }
        AnalysisFrames analysisFrames = new AnalysisFrames(context, owner, mn);
        AbstractInsnNode[] insns = mn.instructions.toArray();
        for (int i = 0; i < insns.length; ++i) {
            AbstractInsnNode insn = insns[i];
            if (true) {
                if (insn instanceof MethodInsnNode) {
                    MethodInsnNode m = (MethodInsnNode)insn;
                    if (insn.getOpcode() == INVOKESTATIC) {
                        if (m.owner.equals("java/lang/Class")
                            && m.name.equals("forName")
                            && m.desc.equals("(Ljava/lang/String;)Ljava/lang/Class;")) {
                            BasicValue arg = analysisFrames.getStackValue(i, 0);
                            if (arg != null) {
                                if (arg instanceof StringValue && ((StringValue)arg).getContents() != null) {
                                    String clazz = ((StringValue) arg).getContents();
                                    //System.out.println("reflected class " + clazz);
                                    minfo.addReferencedClass(clazz);
                                    minfo.addInstantiatedClass(clazz);
                                } else if (arg instanceof ParameterValue || arg instanceof StringBuilderValue) {
                                    minfo.getMethodSummary().addInstantiatedValue(arg);
                                } else {
                                    //System.out.println("! unknown Class.forName at " + owner + " " + mn.name + " " + mn.desc);
                                }
                            }
                            info.addReflectionCaller(minfo);
                        } else if (m.owner.equals("java/util/ServiceLoader")
                            && m.name.equals("load")) {
                            BasicValue arg = analysisFrames.getStackValue(i, 0);
                            // TODO this is not very precise right now so we just load everything if we can't find the service to load
                            // need interprocedural analysis to find the class names that are arriving

                            if (arg != null && arg instanceof ClassValue) {
                                minfo.addReferencedClass(((ClassValue) arg).getName());
                                minfo.addInstantiatedClass(((ClassValue) arg).getName());
                            } else if (arg instanceof ParameterValue) {
                                //System.out.println("ServiceLoader.store is passed parameter " + ((ParameterValue) arg).getIndex());
                                minfo.getMethodSummary().addInstantiatedValue(arg);
                            } else {
                                minfo.addReferencedClass(JMin.ALL_SVC_IMPLEMENTAIONS);
                                minfo.addInstantiatedClass(JMin.ALL_SVC_IMPLEMENTAIONS);
                                /*for (String si : context.getServiceInterfaces()) {
                                    minfo.addReferencedClass(si);
                                    for (String svc : context.getServiceProviders(si)) {
                                        minfo.addReferencedClass(svc);
                                        minfo.addInstantiatedClass(svc);
                                    }
                                }*/
                            }
                            info.addReflectionCaller(minfo);
                        }
                        minfo.addCallSite(m.owner, m.name, m.desc, CallKind.STATIC, i, classSource);

                    } else if (insn.getOpcode() == INVOKEVIRTUAL) {
                        String name = m.name;
                        String desc = m.desc;
                        String clazz = m.owner;
                        boolean doTypeAnalysis = true;
                        /* TODO: Relying on just method name and signature is not entirely correct,
                         * and can result in false positives.
                         * We need a way to identify all class loaders and handle loadClass() method
                         * of such classes only.
                         */
                        if (m.name.equals("loadClass")
                            && m.desc.equals("(Ljava/lang/String;)Ljava/lang/Class;")) {
                            BasicValue arg = analysisFrames.getStackValue(i, 0);
                            if (arg != null && arg instanceof StringValue && ((StringValue)arg).getContents() != null) {
                                minfo.addReferencedClass(((StringValue) arg).getContents());
                            }
                        } else if (m.owner.equals("java/lang/reflect/Method")
                            && m.name.equals("invoke")
                            && m.desc.equals("(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;")) {
                            BasicValue arg = analysisFrames.getStackValue(i, 2);
                            if (arg != null && arg instanceof MethodValue) {
                                MethodValue mv = (MethodValue)arg;
                                clazz = mv.getClazz();
                                name = mv.getName();
                                desc = "*";
                                /* Type analysis should not be done in this case as the the callee has been changed to reflect
                                 * the actual method to be invoked by j.l.r.Method.invoke().
                                 */
                                doTypeAnalysis = false;
                            }
                        }
                        else if (m.owner.equals("java/lang/Class")
                            && m.name.equals("newInstance")
                            && m.desc.equals("()Ljava/lang/Object;")) {
                            BasicValue arg = analysisFrames.getStackValue(i, 0);
                            if (arg != null) {
                                if (arg instanceof ClassValue) {
                                    String init = ((ClassValue) arg).getName();
                                    minfo.addInstantiatedClass(init);
                                } else if (arg instanceof ParameterValue) {
                                    minfo.getMethodSummary().addInstantiatedValue(arg);
                                } else {
                                    //System.out.println("! Unknown Class.newInstance - type information may be incomplete");
                                }
                            }
                            info.addReflectionCaller(minfo);
                        }
                        CallKind kind = CallKind.VIRTUAL;
                        if (doTypeAnalysis) {
                            BasicValue receiver = analysisFrames.getReceiverValue(i, Type.getArgumentTypes(m.desc).length);
                            if (receiver != null) {
                                if (receiver instanceof TypeValue || receiver instanceof FixedTypeValue) {
                                    String refinedClazz = receiver.getType().getInternalName();
                                    if (!refinedClazz.equals(clazz)) {
                                        clazz = refinedClazz;
                                    }
                                }
                                if (receiver instanceof FixedTypeValue) {
                                    kind = CallKind.FIXED;
                                }
                            }
                        }
                        minfo.addCallSite(clazz, name, desc, kind, i, classSource);

                    } else if (insn.getOpcode() == INVOKEINTERFACE) {
                        String clazz = m.owner;
                        CallKind kind = CallKind.INTERFACE;
                        BasicValue receiver = analysisFrames.getReceiverValue(i, Type.getArgumentTypes(m.desc).length);
                        if (receiver != null) {
                            if (receiver instanceof TypeValue || receiver instanceof FixedTypeValue) {
                                String refinedClazz = receiver.getType().getInternalName();
                                if (!refinedClazz.equals(clazz)) {
                                    clazz = refinedClazz;
                                    kind = CallKind.VIRTUAL;
                                }
                            }
                            if (receiver instanceof FixedTypeValue) {
                                kind = CallKind.FIXED;
                            }
                        }
                        minfo.addCallSite(clazz, m.name, m.desc, kind, i, classSource);
                    } else {
                        minfo.addCallSite(m.owner, m.name, m.desc, CallKind.SPECIAL, i, classSource);
                    }
                    /*if (m.owner.startsWith("java/awt") || m.owner.startsWith("javax/swing")) {
                        System.out.println("@^@ " + m.owner + " from typeinsnnode in " + owner + "." + mn.name + mn.desc);
                    }*/
                } else if (insn instanceof TypeInsnNode) {
                    TypeInsnNode t = (TypeInsnNode)insn;
                    /*if (t.desc.startsWith("java/awt") || t.desc.startsWith("javax/swing")) {
                        System.out.println("@^@ " + t.desc + " from typeinsnnode in " + owner + "." + mn.name + mn.desc);
                    }*/
                    minfo.addReferencedClass(t.desc);
                    if (t.getOpcode() == NEW) {
                        minfo.addInstantiatedClass(t.desc);
                    }
                } else if (insn instanceof FieldInsnNode) {
                    FieldInsnNode f = (FieldInsnNode)insn;
                    minfo.addReferencedField(f.owner, f.name, f.desc, f.getOpcode() == GETSTATIC || f.getOpcode() == PUTSTATIC ? FieldKind.STATIC : FieldKind.INSTANCE);
                } else if (insn instanceof LdcInsnNode) {
                    LdcInsnNode l = (LdcInsnNode)insn;
                    if (l.cst instanceof Type) {
                        Type t = (Type)l.cst;
                        if (t.getSort() == OBJECT) {
                            /*if (t.getInternalName().startsWith("java/awt") || t.getInternalName().startsWith("javax/swing")) {
                                    System.out.println("@^@ " + t.getInternalName() + " from array ldc in " + owner + "." + mn.) {
                                System.out.println("@^@ " + t.getInternalName() + " from class ldc in " + owner + "." + mn.name + mn.desc);
                            }*/
                            minfo.addReferencedClass(t.getInternalName());
                        } else if (t.getSort() == ARRAY) {
                            do {
                                t = t.getElementType();
                            } while (t.getSort() == ARRAY);
                            if (t.getSort() == OBJECT) {
                                /*if (t.getInternalName().startsWith("java/awt") || t.getInternalName().startsWith("javax/swing")) {
                                    System.out.println("@^@ " + t.getInternalName() + " from array ldc in " + owner + "." + mn.name + mn.desc);
                                }*/
                                minfo.addReferencedClass(t.getInternalName());
                            }
                        }
                    }
                } else if (insn instanceof InvokeDynamicInsnNode) {
                    InvokeDynamicInsnNode indy = (InvokeDynamicInsnNode)insn;
                    Handle h = indy.bsm;
                    if (h.getOwner().equals("java/lang/invoke/LambdaMetafactory")
                        && (h.getName().equals("metafactory") || h.getName().equals("altMetafactory"))
                        && indy.bsmArgs.length > 2
                        && indy.bsmArgs[1] instanceof Handle) {
                        Handle hBSM = (Handle)indy.bsmArgs[1];
                        if (hBSM.getName().equals("<init>")) {
                            minfo.addReferencedClass(hBSM.getOwner());
                            minfo.addInstantiatedClass(hBSM.getOwner());
                        }
                        minfo.addCallSite(hBSM.getOwner(), hBSM.getName(), hBSM.getDesc(), CallKind.DYNAMIC, i, classSource);
                    } else {
                        System.out.println("indy name " + indy.name + " desc " + indy.desc);
                        System.out.println("  handle " + h.getOwner() + " " + h.getName() + " " + h.getDesc());
                        for (Object o : indy.bsmArgs) {
                            System.out.println("  " + o.getClass().getName() + ":" + o.toString());
                        }
                        //throw new RuntimeException("Unknown indy");
                    }
                }
            }
        }
    }
}

class AnalysisFrames {
    private Frame<BasicValue>[] frames;
    private Frame<BasicValue>[] typeFrames;
    private HierarchyContext context;
    private String owner;
    private MethodNode mn;
    public AnalysisFrames(HierarchyContext context, String owner, MethodNode mn) {
        this.context = context;
        this.owner = owner;
        this.mn = mn;
    }
    public BasicValue getStackValue(int instructionIndex, int frameIndex) throws AnalyzerException {
        if (frames == null) {
            InterospectiveAnalyzer a = new InterospectiveAnalyzer(new ReflectionInterpreter());
            a.analyze(owner, mn);
            frames = a.getFrames();
        }
        Frame<BasicValue> f = frames[instructionIndex];
        if (f == null) {
            return null;
        }
        int top = f.getStackSize() - 1;
        return frameIndex <= top ? f.getStack(top - frameIndex) : null;
    }
    public BasicValue getReceiverValue(int instructionIndex, int frameIndex) throws AnalyzerException {
        // Note: using the type interpreter here during the first pass over the classes can result
        // in less preise type information than we otherwise would be able to have if the hierarchy
        // was fully built. We have to be careful how we use the context info, but this use should be
        // safe and helps even if we don't manage to limit the types for every callsite as much as we
        // might otherwise.
        if (!Config.enableTypeRefinement) {
            return null;
        }
        if (typeFrames == null) {
            Analyzer<BasicValue> a = new Analyzer<BasicValue>(new TypeInterpreter(context));
            a.analyze(owner, mn);
            typeFrames = a.getFrames();
        }
        Frame<BasicValue> f = typeFrames[instructionIndex];
        if (f == null) {
            return null;
        }
        int top = f.getStackSize() - 1;
        return frameIndex <= top ? f.getStack(top - frameIndex) : null;
    }
}
