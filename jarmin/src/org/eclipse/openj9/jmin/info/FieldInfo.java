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

package org.eclipse.openj9.jmin.info;

import java.util.HashSet;

public class FieldInfo {
    private final  String name;
    private final String desc;
    private HashSet<String> annotations;
    boolean referenced;

    public FieldInfo(String name, String desc) {
        this.name = name;
        this.desc = desc;
        this.referenced = false;
        this.annotations = new HashSet<String>();
    }
    public String name() {
        return name;
    }
    public String desc() {
        return desc;
    }
    public void setReferenced() {
        this.referenced = true;
    }
    public boolean referenced() {
        return referenced;
    }
    public void addAnnotation(String annot) {
        annotations.add(annot);
    }
    public HashSet<String> getAnnotations() {
        return annotations;
    }
    @Override
    public String toString() {
        return name + desc + " -" + (referenced ? "" : " not") + " referenced";
    }
}