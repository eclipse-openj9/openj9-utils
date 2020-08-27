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

import java.util.ArrayList;

import org.objectweb.asm.Type;
import org.objectweb.asm.tree.analysis.BasicValue;

public class StringBuilderValue extends BasicValue {
    private ArrayList<BasicValue> contents;
    private boolean appended;
    
    public StringBuilderValue() {
        super(Type.getObjectType("java/lang/String"));
        this.contents = new ArrayList<BasicValue>();
        this.appended = false;
    }
    
    public StringBuilderValue(StringBuilderValue v) {
        super(Type.getObjectType("java/lang/String"));
        this.contents = new ArrayList<BasicValue>(v.contents);
        this.appended = false;
    }
    
    public void append(BasicValue v) {
        //assert !appended : "Cannot append more than once to a StringBuilderValue";
        appended = true;
        contents.add(v);
    }

    public void append(ArrayList<BasicValue> values) {
        //assert !appended : "Cannot append more than once to a StringBuilderValue";
        appended = true;
        contents.addAll(values);
    }

    public BasicValue getContents() {
        if (isComputable()) {
            StringBuilder builder = new StringBuilder();
            for (BasicValue bv : contents) {
                String toAppend = ((StringValue) bv).getContents();
                builder.append(toAppend);
            }
            return new StringValue(builder.toString());
        } else {
            return new StringBuilderValue(this);
        }
    }

    public boolean isParameterDependent() {
        for (BasicValue v : contents) {
            if (v instanceof ParameterValue) {
                return true;
            }
        }
        return false;
    }

    public StringBuilderValue copyWithParamSubsitution(ParameterValue param, BasicValue value) {
        StringBuilderValue toReturn = new StringBuilderValue();
        toReturn.contents.addAll(toReturn.contents);
        for (int i = 0, e = toReturn.contents.size(); i != e; ++i) {
            if (toReturn.contents.get(i).equals(param)) {
                toReturn.contents.set(i, value);
            }
        }
        return toReturn;
    }

    public StringBuilderValue copyWithParamSubstituition(BasicValue[] paramValues) {
        StringBuilderValue toReturn = new StringBuilderValue();
        for (int i = 0; i != contents.size(); i++) {
            if (contents.get(i) instanceof ParameterValue) {
                ParameterValue parameter = (ParameterValue)contents.get(i);
                BasicValue paramValue = paramValues[parameter.getIndex()];
                if (paramValue instanceof StringBuilderValue) {
                    toReturn.append(((StringBuilderValue) paramValue).contents);
                } else if (paramValue instanceof ParameterValue || paramValue instanceof StringValue) {
                    toReturn.append(paramValue);
                } else {
                    return null;
                }
            } else {
                toReturn.append(contents.get(i));
            }
        }
        return toReturn;
    }

    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null) {
            return false;
        }
        if (o instanceof StringBuilderValue) {
            ArrayList<BasicValue> ocontents = ((StringBuilderValue)o).contents;
            if (ocontents == contents) {
                return true;
            }
            if (ocontents != null && contents != null && ocontents.size() == contents.size()) {
                for (int i = 0, e = contents.size(); i != e; ++i) {
                    if (!contents.get(i).equals(ocontents.get(i))) {
                        return false;
                    }
                }
                return true;
            }
        }
        return false;
    }

    public boolean isComputable() {
        for (BasicValue entry: contents) {
            if (!(entry instanceof StringValue)) {
                return false;
            }
        }
        return true;
    }
}