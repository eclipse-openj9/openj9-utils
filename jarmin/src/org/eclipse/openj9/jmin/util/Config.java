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

package org.eclipse.openj9.jmin.util;

public class Config {
    public static final String REDUCTION_MODE_PROPERTY_NAME = "org.eclipse.openj9.jmin.reduction_mode";
    public static final String INCLUSION_MODE_PROPERTY_NAME = "org.eclipse.openj9.jmin.inclusion_mode";
    public static final String TRACE_PROPERTY_NAME = "org.eclipse.openj9.jmin.trace";
    public static final String ENABLE_METHOD_SUMMARY = "org.eclipse.openj9.jmin.method_summary.enable";
    public static final String ENABLE_TYPE_REFINEMENT = "org.eclipse.openj9.jmin.type_refinement.enable";
    public static final String ENABLE_INNER_JAR_PROCESSING = "org.eclipse.openj9.jmin.inner_jar_processing.enable";

    /* Valid values for reductionMode */
    public static final int REDUCTION_MODE_CLASS = 1;
    public static final int REDUCTION_MODE_METHOD = 2;
    public static final int REDUCTION_MODE_FIELD = 3;

    /* Valid values for inclusionMode */
    public static final int INCLUSION_MODE_REFERENCE = 1;
    public static final int INCLUSION_MODE_INSTANTIATE = 2;

    public static int reductionMode;
    public static int inclusionMode;
    public static boolean enableMethodSummary;
    public static boolean enableTypeRefinement;
    public static boolean trace;
    public static boolean enableInnerJarProcessing;

    public static void setGlobalConfig() {
        reductionMode = REDUCTION_MODE_CLASS;
        String reductionModeValue = System.getProperty(REDUCTION_MODE_PROPERTY_NAME);
        if (reductionModeValue != null) {
            if (reductionModeValue.equalsIgnoreCase("class")) {
                reductionMode = REDUCTION_MODE_CLASS;
            } else if (reductionModeValue.equalsIgnoreCase("method")) {
                reductionMode = REDUCTION_MODE_METHOD;
            } else if (reductionModeValue.equalsIgnoreCase("field")) {
                reductionMode = REDUCTION_MODE_FIELD;
            }
        }

        inclusionMode = INCLUSION_MODE_INSTANTIATE;
        String inclusionModeValue = System.getProperty(INCLUSION_MODE_PROPERTY_NAME);
        if (inclusionModeValue != null) {
            if (inclusionModeValue.equalsIgnoreCase("reference")) {
                inclusionMode = INCLUSION_MODE_REFERENCE;
            }
        }

        String traceMode = System.getProperty(TRACE_PROPERTY_NAME);
        trace = traceMode != null && traceMode.equalsIgnoreCase("true");

        enableMethodSummary = true;
        String methodSummary = System.getProperty(ENABLE_METHOD_SUMMARY);
        if (methodSummary != null && methodSummary.equalsIgnoreCase("false")) {
            enableMethodSummary = false;
        }

        enableTypeRefinement = true;
        String typeRefinement = System.getProperty(ENABLE_TYPE_REFINEMENT);
        if (typeRefinement != null && typeRefinement.equalsIgnoreCase("false")) {
            enableTypeRefinement = false;
        }

        enableInnerJarProcessing = true;
        String innerJarProcessing = System.getProperty(ENABLE_INNER_JAR_PROCESSING);
        if (innerJarProcessing != null && innerJarProcessing.equalsIgnoreCase("false")) {
            enableInnerJarProcessing = false;
        }
    }

    public static boolean validateProperties() {
        boolean argsValid = true;
        String reductionMode = System.getProperty(REDUCTION_MODE_PROPERTY_NAME);
        argsValid &= reductionMode == null || reductionMode.equals("class") || reductionMode.equals("method") || reductionMode.equals("field");
        String inclusionMode = System.getProperty(INCLUSION_MODE_PROPERTY_NAME);
        argsValid &= inclusionMode == null || inclusionMode.equals("reference") || inclusionMode.equals("instantiation");
        String traceMode = System.getProperty(TRACE_PROPERTY_NAME);
        argsValid &= traceMode == null || traceMode.equalsIgnoreCase("true") || traceMode.equalsIgnoreCase("false");
        String methodSummary = System.getProperty(ENABLE_METHOD_SUMMARY);
        argsValid &= methodSummary == null || methodSummary.equalsIgnoreCase("true") || methodSummary.equalsIgnoreCase("false");
        return argsValid;
    }

    public static void showCurrentConfig() {
        System.out.println("Current configuration:");
        System.out.println("\tReduction mode: " + reductionMode);
        System.out.println("\tInclusion mode: " + inclusionMode);
        System.out.println("\tTrace mode: " + trace);
        System.out.println("\tMethod summary enabled: " + enableMethodSummary);
        System.out.println("\tType refinement enabled: " + enableTypeRefinement);
    }
}
