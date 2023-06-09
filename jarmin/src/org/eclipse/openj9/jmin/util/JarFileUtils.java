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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

package org.eclipse.openj9.jmin.util;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;

public class JarFileUtils {
    private static JarInputStream getInnerMostJarInputStream(String jarFile) {
        JarInputStream is = null;
        String[] parts = jarFile.split("!/");
        try {
            is = new JarInputStream(new FileInputStream(parts[0]));
            for (int i = 1; i < parts.length; i++) {
                String entryName = parts[i];
                JarEntry entry = is.getNextJarEntry();
                while (entry != null && !entry.getName().equals(entryName)) {
                    entry = is.getNextJarEntry();
                }
                assert entry != null : "Failed to find jar entry " + entryName + "in jar file " + jarFile;
                is = new JarInputStream(is);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return is;
    }

    public static InputStream getJarEntryInputStream(String jarFile, String jarEntry) {
        JarInputStream jin = getInnerMostJarInputStream(jarFile);
        try {
            JarEntry entry = jin.getNextJarEntry();
            while (entry != null && !entry.getName().equals(jarEntry)) {
                entry = jin.getNextJarEntry();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return jin;
    }
}
