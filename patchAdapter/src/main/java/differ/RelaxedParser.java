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

package differ;

import java.util.List;
import java.util.ArrayList;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.DefaultParser;

public class RelaxedParser extends DefaultParser {

    private static List<String> leftoverArgs = new ArrayList<String>();

    @Override
    public CommandLine parse(Options options, String[] arguments) throws ParseException {
        leftoverArgs = new ArrayList<String>(); // leftovers are on a per parse basis
        List<String> knownArguments = new ArrayList<>();
        // this is only ok bc we know all of our options have an arg
        for (int i = 0; i < arguments.length; i++) {
            if (options.hasOption(arguments[i])) {
                knownArguments.add(arguments[i]);
                knownArguments.add(arguments[i + 1]);
                i++;
            } else {
                leftoverArgs.add(arguments[i]);
            }
        }
        return super.parse(options, knownArguments.toArray(new String[knownArguments.size()]));
    }

    // doesnt simply return the reference, others could then modify
    public void getLeftovers(List<String> newUnknowns) {
        for (String arg : leftoverArgs) {
            newUnknowns.add(arg);
        }
    }
}
