{{- /*******************************************************************************
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
 *******************************************************************************/ -}}
import org.objectweb.asm.*;
import java.io.IOException;

public class DebugClassWriter {
    private ClassReader reader;
    private ClassWriter writer;
    private static String INVOKE = "invoke";


    public DebugClassWriter(String className) throws IOException {
        reader = new ClassReader(className);
        writer = new ClassWriter(reader, ClassWriter.COMPUTE_FRAMES);
    }

    public byte[] applyDebugger(String methodName){
        if (methodName == INVOKE) {
            MethodDebugAdapter methodDebugAdapter = new MethodDebugAdapter(methodName, writer);
            reader.accept(methodDebugAdapter, 0);
        } else {
            ExpectDebugAdapter expectDebugAdapter = new ExpectDebugAdapter(methodName, writer);
            reader.accept(expectDebugAdapter, 0);
        }

        return writer.toByteArray();
    }
}