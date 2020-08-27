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

import org.objectweb.asm.tree.analysis.BasicValue;

import org.objectweb.asm.Type;

/**
 * Represents a constant string value propagating through the program.
 * 
 * The StringValue can have a null contents which represents a String object that has not
 * had its contents set yet.
 */
public class StringValue extends BasicValue {
    private String contents;
    
    public StringValue() {
        super(Type.getObjectType("java/lang/String"));
        this.contents = null;
    }
    
    public StringValue(String contents) {
        super(Type.getObjectType("java/lang/String"));
        this.contents = contents;
    }
    
    public StringValue(StringValue v) {
        super(Type.getObjectType("java/lang/String"));
        this.contents = new String(v.getContents());
    }
    
    public String getContents() {
        return contents;
    }
    
    public void setContents(String contents) {
        if (this.contents != null) {
            this.contents = null;
        } else {
            this.contents = contents;
        }
    }
    
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null) {
            return false;
        }
        if (o instanceof StringValue) {
            String ocontents = ((StringValue)o).contents;
            return (ocontents == contents) || (ocontents != null && contents != null && contents.equals(ocontents));
        }
        return false;
    }
}