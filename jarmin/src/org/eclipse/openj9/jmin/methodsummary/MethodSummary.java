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

package org.eclipse.openj9.jmin.methodsummary;

import org.objectweb.asm.tree.analysis.BasicValue;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class MethodSummary {
    ArrayList<BasicValue> instantiatedValues;

    public void addInstantiatedValue(BasicValue iv) {
        if (instantiatedValues == null) {
            instantiatedValues = new ArrayList<BasicValue>();
        } else {
            for (BasicValue bv : instantiatedValues) {
                if (bv.equals(iv)) return;
            }
        }
        instantiatedValues.add(iv);
    }

    public List<BasicValue> getInstantiatedValues() {
        if (instantiatedValues != null) {
            return Collections.unmodifiableList(instantiatedValues);
        } else {
            return null;
        }
    }
}
