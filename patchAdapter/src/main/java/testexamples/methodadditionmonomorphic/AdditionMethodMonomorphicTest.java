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

package testexamples.methodadditionmonomorphic;

public class AdditionMethodMonomorphicTest {

    public int field;
    
    public int returnSamePlusSeven(int x){
	field = 7;
	int temp = field;
	//int temp = newMethod(x);
	//thirdNewMethod(); //test for ref replacement, not result
	return temp;
    }

    public void testPrinter(){
	System.out.println("Hello World!");
    }

    /*
    public int newMethod(int y){
	testPrinter();
	secondNewMethod(); //test for ref replacement, not result
	return field + y;
    }

    public void secondNewMethod(){
	System.out.println("This is second new method!");
    }

    public static void thirdNewMethod(){
	System.out.println("This is third new method!");
    }
    */
}
