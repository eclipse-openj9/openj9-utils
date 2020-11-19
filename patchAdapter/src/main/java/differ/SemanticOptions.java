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

import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;

public class SemanticOptions extends Options {

    public SemanticOptions() {
        Option redefinitionClass = Option.builder().longOpt("redefcp").hasArg().required()
                .desc("Specify the classpath of the redefinition classes.").build();
        addOption(redefinitionClass);

        Option originalClass = Option.builder().longOpt("originalclasslist").hasArg()
                .desc("Specify a file containing the list of original classes.").build();
        addOption(originalClass);

        Option firstDest = Option.builder().longOpt("renameDestination").hasArg()
                .desc("Specify the first directory for the renamed class to go to.").build();
        addOption(firstDest);

        Option altDest = Option.builder().longOpt("finalDestination").hasArg()
                .desc("Specify the directory for the adapter output to go to.").build();
        addOption(altDest);

        // bit odd to have arg, but bc of parsing strategy in relaxed parser to sort our
        // opts from soot opts.
        Option runRename = Option.builder().longOpt("runRename").hasArg()
                .desc("Specify whether the rename phase should be run. Default false.").build();
        addOption(runRename);

        Option mainClass = Option.builder().longOpt("mainClass").hasArg().desc(
                "Specify the actual main class of the application, to be provided to Soot AFTER Soot startup has occurred.")
                .build();
        addOption(mainClass);

        Option differClasspath = Option.builder().longOpt("differClasspath").hasArg().desc(
                "Specify the classpath for the patch adapter's run of Soot. May be different than that of a Cogni+Soot run, if patch adapter is run from HOTFIXER.")
                .build();
        addOption(differClasspath);

        Option useFullDir = Option.builder().longOpt("useFullDir").hasArg()
                .desc("Specify whether Soot's process-dir option should be used or not.").build();
        addOption(useFullDir);

    }
}
