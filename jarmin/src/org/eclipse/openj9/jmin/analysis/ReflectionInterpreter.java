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

import java.util.List;

import org.objectweb.asm.Type;
import org.objectweb.asm.tree.*;
import org.objectweb.asm.tree.analysis.*;

import static org.objectweb.asm.tree.analysis.BasicValue.REFERENCE_VALUE;

public class ReflectionInterpreter extends BasicInterpreter {
    public ReflectionInterpreter() {
        super(ASM8);
    }

    @Override
    public BasicValue newOperation(AbstractInsnNode insn) throws AnalyzerException
    {
      if (insn instanceof LdcInsnNode) {
          LdcInsnNode ldc = (LdcInsnNode)insn;
          Object cst = ldc.cst;
          if (cst instanceof String) {
              return new StringValue((String)cst);
          } else if (cst instanceof Type) {
              return new ClassValue(((Type)cst).getInternalName());
          }
      } else if (insn instanceof TypeInsnNode && insn.getOpcode() == NEW) {
          TypeInsnNode n = (TypeInsnNode)insn;
          if (n.desc.equals("java/lang/String")) {
              return new StringValue(); // contents will be set later (potentially)
          } else if (n.desc.equals("java/lang/StringBuilder")) {
              return new StringBuilderValue(); // appender will create new values later
          }
      }
      return super.newOperation(insn);
    }

    @Override
    public BasicValue naryOperation(AbstractInsnNode insn, List<? extends BasicValue> values) throws AnalyzerException {
        return this.naryOperation(insn, values, null);
    }
    public BasicValue naryOperation(AbstractInsnNode insn, List<? extends BasicValue> values, final InterospectiveFrame frame) throws AnalyzerException {
        if (insn instanceof MethodInsnNode) {
            MethodInsnNode m = (MethodInsnNode) insn;
            if (m.getOpcode() == INVOKESPECIAL) {
                // handle string constructors
                if (m.owner.equals("java/lang/String")
                        && m.name.equals("<init>")) {
                    if (m.desc.equals("()V")
                            && values.get(0) instanceof StringValue) {
                        ((StringValue) values.get(0)).setContents("");
                    } else if (m.desc.equals("(Ljava/lang/String;)V")
                            && values.get(0) instanceof StringValue
                            && values.get(1) instanceof StringValue) {
                        ((StringValue) values.get(0)).setContents(((StringValue) values.get(1)).getContents());
                    }
                }
            } else if (m.getOpcode() == INVOKESTATIC) {
                if (m.owner.equals("java/lang/Class")
                        && m.name.equals("forName")
                        && m.desc.equals("(Ljava/lang/String;)Ljava/lang/Class;")
                        && values.get(0) instanceof StringValue
                        && ((StringValue) values.get(0)).getContents() != null) {
                    return new ClassValue(((StringValue) values.get(0)).getContents());
                }
            } else if (m.getOpcode() == INVOKEVIRTUAL) {
                /* TODO: Relying on just method name and signature is not entirely correct,
                 * and can result in false positives.
                 * We need a way to identify all class loaders and handle loadClass() method
                 * of such classes only.
                 */
                if (m.name.equals("loadClass")
                        && m.desc.equals("(Ljava/lang/String;)Ljava/lang/Class;")
                        && values.get(1) instanceof StringValue
                        && ((StringValue) values.get(1)).getContents() != null) {
                    return new ClassValue(((StringValue) values.get(1)).getContents());
                } else if (m.owner.equals("java/lang/Class")) {
                    if ((m.name.equals("getMethod") || m.name.equals("getDeclaredMethod"))
                            && m.desc.equals("(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;")
                            && values.get(0) instanceof ClassValue
                            && values.get(1) instanceof StringValue
                            && ((StringValue) values.get(1)).getContents() != null) {
                        return new MethodValue(((ClassValue) values.get(0)).getName(), ((StringValue) values.get(1)).getContents(), "*");
                    } else if ((m.name.equals("getField") || m.name.equals("getDeclaredField"))
                            && m.desc.equals("(Ljava/lang/String;)Ljava/lang/reflect/Field;")
                            && values.get(0) instanceof ClassValue
                            && values.get(1) instanceof StringValue
                            && ((StringValue) values.get(1)).getContents() != null) {
                        return new FieldValue(((ClassValue) values.get(0)).getName(), ((StringValue) values.get(1)).getContents());
                    }
                } else if (m.owner.equals("java/lang/StringBuilder")) {
                    if (m.name.equals("append")) {
                        if (m.desc.equals("(Ljava/lang/String;)Ljava/lang/StringBuilder;")
                            && values.get(0) != null
                            && values.get(0) instanceof StringBuilderValue
                            && values.get(1) != null
                            && (values.get(1) instanceof StringValue || values.get(1) instanceof ParameterValue)) {
                            StringBuilderValue sbv = new StringBuilderValue((StringBuilderValue) values.get(0));
                            sbv.append(values.get(1));
                            if (frame != null) {
                                frame.replaceValue(values.get(0), sbv);
                            }
                            return sbv;
                        }
                    } else if (m.name.equals("toString") && m.desc.equals("()Ljava/lang/String;")) {
                        if (values.get(0) instanceof StringBuilderValue) {
                            return ((StringBuilderValue) values.get(0)).getContents();
                        }
                    }
                }
            }
        }
        return super.naryOperation(insn, values);
    }

    @Override
    public BasicValue newParameterValue(final boolean isInstanceMethod, final int local, final Type type) {
        if (isInstanceMethod) {
            return new ParameterValue(local-1, newValue(type));
        } else {
            return new ParameterValue(local, newValue(type));
        }
    }

    @Override
    public BasicValue merge(BasicValue v1, BasicValue v2) {
        if (v1 instanceof StringValue
            && v2 instanceof StringValue
            && v1.equals(v2)) {
            return new StringValue((StringValue)v1);
        } else if (v1 instanceof ClassValue
                   && v2 instanceof ClassValue
                   && v1.equals(v2)) {
            return new ClassValue((ClassValue)v1);
        } else if (v1 instanceof MethodValue
                   && v2 instanceof MethodValue
                   && v1.equals(v2)) {
            return new MethodValue((MethodValue)v2);
        } else if (v1 instanceof StringBuilderValue
                   && v2 instanceof StringBuilderValue
                   && v1.equals(v2)) {
            return new StringBuilderValue((StringBuilderValue)v1);
        } else if (v1 instanceof ParameterValue
                   && v2 instanceof ParameterValue) {
            if (((ParameterValue)v1).equals(v2)) {
                return new ParameterValue((ParameterValue)v2);
            }
            return this.merge(((ParameterValue)v1).getValue(), ((ParameterValue)v2).getValue());
        }
        return super.merge(degradeValue(v1), degradeValue(v2));
    }

    private BasicValue degradeValue(BasicValue v) {
        if (v instanceof StringValue || v instanceof ClassValue || v instanceof MethodValue || v instanceof StringBuilderValue) {
            return REFERENCE_VALUE;
        }
        if (v instanceof ParameterValue) {
            return degradeValue(((ParameterValue)v).getValue());
        }
        return v;
    }
}
