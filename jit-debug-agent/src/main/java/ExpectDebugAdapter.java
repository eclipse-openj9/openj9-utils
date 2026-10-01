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

public class ExpectDebugAdapter extends ClassVisitor {
    private String methodName;
    private int access = org.objectweb.asm.Opcodes.ACC_PUBLIC;

    public ExpectDebugAdapter(String methodName, ClassVisitor cv) {
        super(org.objectweb.asm.Opcodes.ASM9, cv);
        this.cv = cv;
        this.methodName = methodName;
    }

    @Override
    public MethodVisitor visitMethod(
            int access,
            String name,
            String desc,
            String signature,
            String[] exceptions) {
        MethodVisitor mv;
        mv =  cv.visitMethod(
                access,
                name,
                desc,
                signature,
                exceptions);
        if (name.equals(methodName) && mv != null) {
            return new EvaluateDebugAdapter(mv);
        }
        return mv;
    }
}