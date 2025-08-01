/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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
import org.objectweb.asm.*;

public class EvaluateDebugAdapter extends MethodVisitor {

    public EvaluateDebugAdapter(MethodVisitor mv) {
        super(Opcodes.ASM9, mv);
        this.mv = mv;
    }

    @Override
    public void visitCode() {
        mv.visitCode();
        mv.visitVarInsn(Opcodes.ALOAD, 0);
        mv.visitFieldInsn(Opcodes.GETFIELD, "org/junit/internal/runners/statements/ExpectException", "expected", "Ljava/lang/Class;");
        mv.visitMethodInsn(Opcodes.INVOKESTATIC, "com/ibm/jit/JITHelpers", "setExpectedException", "(Ljava/lang/Class;)V", false);
    }

    @Override
    public void visitLineNumber(int line, Label start){
        mv.visitLineNumber(line, start);
        if (line == 31) {
            mv.visitInsn(Opcodes.ACONST_NULL);
            mv.visitMethodInsn(Opcodes.INVOKESTATIC, "com/ibm/jit/JITHelpers", "setExpectedException", "(Ljava/lang/Class;)V", false);
        }
    }

}