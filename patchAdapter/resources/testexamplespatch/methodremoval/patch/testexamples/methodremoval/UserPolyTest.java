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

package testexamples.methodremoval;

public class UserPolyTest {

    public int use(int decision){
	AdditionMethodPolymorphicTestParent p;
	if(decision == 1){
	    p = new AdditionMethodPolymorphicTestChildOne();
	} else if(decision == 2) {
	    p =	new AdditionMethodPolymorphicTestChildTwo();
	} else if(decision == 3) {
	    p = new AdditionMethodPolymorphicTestChildThree();
	} else if(decision == 4) {
	    p = new AdditionMethodPolymorphicTestChildFour();
	} else if(decision == 5) {
	    p = new AdditionMethodPolymorphicTestChildFive();
	} else if(decision == 6) {
	    p = new AdditionMethodPolymorphicTestChildSix();
	} else {
	    p = new AdditionMethodPolymorphicTestParent();
	}
	return p.emitMethod();
    }

}
