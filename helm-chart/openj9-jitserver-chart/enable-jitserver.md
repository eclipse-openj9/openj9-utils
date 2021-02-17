<!--
Copyright (c) 2020, 2021 IBM Corp. and others

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] http://openjdk.java.net/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
-->

# Quick Start: Enabling JITServer Technology

This quick start guide walks you through an example of enabling JITServer Technology for your java application. This guide uses [Open Liberty Application Server](https://openliberty.io) as a Java application. JITServer technology can be used for any Java application running on recent releases of OpenJ9 for Java 8 and Java 11. Please make sure that JITServer image is compatible with the JDK that runs your application, as described [here](./README.md#a-verify-the-application-jdk-version-and-select-the-appropriate-jitserver-image).

You will be provided with a basic understanding of JITServer technology and how it works with your java application. 

## Installing Open Liberty Application Server

This quick start guide uses Open Liberty Application Server as an example Java application, and shows how to enable JITServer technology even after your Java application is deployed. 

Please follow Open Liberty Helm Chart [Installation Guide](https://github.com/IBM/charts/tree/master/stable/ibm-open-liberty#installing-the-chart) to install Open Liberty on your cluster. 

* Set helm repo
Set helm repo to `IBM/charts`.

``` bash
$ helm repo add ibm-charts https://raw.githubusercontent.com/IBM/charts/master/repo/stable/
"ibm-charts" has been added to your repositories
```

* Install Open Liberty Helm Chart
Install the official Open Liberty Helm Chart on your cluster and set application image to an OpenJ9 based image. 

``` bash
$ helm install liberty --set image.tag=kernel-java8-openj9-ubi ibm-charts/ibm-open-liberty
NAME: liberty
LAST DEPLOYED: Wed Oct  7 19:29:13 2020
NAMESPACE: helm-chart-test
STATUS: deployed
REVISION: 1
```

* Verify Open Liberty Application Deployment
An Open Liberty instance (`liberty-ibm-open-liberty`) should be running on your cluster. 

``` bash
$ kubectl get all
NAME                                            READY     STATUS    RESTARTS   AGE
pod/liberty-ibm-open-liberty-566b477b4b-f6jwz   0/1       Running   0          20s

NAME                               TYPE       CLUSTER-IP      EXTERNAL-IP   PORT(S)          AGE
service/liberty-ibm-open-liberty   NodePort   172.30.15.108   <none>        9443:30130/TCP   20s

NAME                                       READY     UP-TO-DATE   AVAILABLE   AGE
deployment.apps/liberty-ibm-open-liberty   0/1       1            0           20s

NAME                                                  DESIRED   CURRENT   READY     AGE
replicaset.apps/liberty-ibm-open-liberty-566b477b4b   1         1         0         20s
```

## Installing JITServer

* Install OpenJ9 JITServer Helm Chart
Download this chart and follow below steps to install OpenJ9 JITServer on your cluster. 

``` bash
$ cd <OPENJ9_CHART_DIRECTORY>

$ helm install jitserver openj9-jitserver-chart/
NAME: jitserver
LAST DEPLOYED: Wed Oct  7 19:34:48 2020
NAMESPACE: helm-chart-test
STATUS: deployed
REVISION: 1
```

* Verify JITServer and java application is running
An OpenJ9 JITServer instance (`jitserver-openj9-jitserver-chart`) should be running on your cluster. Notice that JITServer service name is `jitserver-openj9-jitserver-chart`. 

``` bash
$ kubectl get all
NAME                                            READY     STATUS                       RESTARTS   AGE
pod/jitserver-openj9-jitserver-chart-55657864-ql8fl       1/1       Running                      0          83s
pod/liberty-ibm-open-liberty-6b9c7b49d7-lzzkg   1/1       Running                      0          3m8s

NAME                               TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)          AGE
service/jitserver-openj9-jitserver-chart     ClusterIP   172.30.249.11   <none>        38400/TCP        83s
service/liberty-ibm-open-liberty   NodePort    172.30.164.79   <none>        9443:32513/TCP   3m8s

NAME                                       READY     UP-TO-DATE   AVAILABLE   AGE
deployment.apps/jitserver-openj9-jitserver-chart     1/1       1            0           83s
deployment.apps/liberty-ibm-open-liberty   1/1       1            0           3m8s

NAME                                                  DESIRED   CURRENT   READY     AGE
replicaset.apps/jitserver-openj9-jitserver-chart-55657864       1         1         0         83s
replicaset.apps/liberty-ibm-open-liberty-6b9c7b49d7   1         1         0         3m8s
```

## Upgrade the open-liberty instance to use JITServer for JIT compilations

* Update Java Application Instance
Set JVM environment variable `jvmArgs` to enable JITServer technology. Replace `<REPLACE_JITSERVER_SERVICE_NAME>` with JITServer service name by checking `kubectl get service`

``` bash
jvmArgs = "-XX:+UseJITServer -XX:JITServerAddress=<REPLACE_JITSERVER_SERVICE_NAME>"
```

* Re-deploy Java Application
Apply changes to `jvmArgs` and upgrade java application. 

``` bash
$ helm upgrade liberty ibm-charts/ibm-open-liberty --set env.jvmArgs="-XX:+UseJITServer -XX:JITServerAddress=<REPLACE_JITSERVER_SERVICE_NAME>"
Release "liberty" has been upgraded. Happy Helming!
NAME: liberty
LAST DEPLOYED: Wed Oct  7 19:40:47 2020
NAMESPACE: helm-chart-test
STATUS: deployed
REVISION: 2
```

* Verify re-deployment is successful
At this time, java application is restarted and JITServer is enabled. 

``` bash
$ kubectl get all
NAME                                            READY     STATUS                       RESTARTS   AGE
pod/jitserver-openj9-jitserver-chart-55657864-ql8fl       1/1       Running                      0          7m50s
pod/liberty-ibm-open-liberty-68df747878-8f5cv   1/1       Running                      0          110s

NAME                               TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)          AGE
service/jitserver-openj9-jitserver-chart     ClusterIP   172.30.249.11   <none>        38400/TCP        7m50s
service/liberty-ibm-open-liberty   NodePort    172.30.164.79   <none>        9443:32513/TCP   9m35s

NAME                                       READY     UP-TO-DATE   AVAILABLE   AGE
deployment.apps/jitserver-openj9-jitserver-chart     1/1       1            0           7m50s
deployment.apps/liberty-ibm-open-liberty   1/1       1            1           9m35s

NAME                                                  DESIRED   CURRENT   READY     AGE
replicaset.apps/jitserver-openj9-jitserver-chart-55657864       1         1         1         7m50s
replicaset.apps/liberty-ibm-open-liberty-68df747878   1         1         1         110s
replicaset.apps/liberty-ibm-open-liberty-6b9c7b49d7   0         0         0         9m35s
```

## Cleanup 

Follow the steps below to remove OpenJ9 JITServer and Open Liberty helm charts from your cluster.

``` bash
$ helm delete jitserver liberty
release "jitserver" uninstalled
release "liberty" uninstalled
```

## Limitations 

* This tutorial requires [Helm v3 CLI](https://helm.sh/blog/helm-3-released/) installed to the cluster. 
* The client JVM and the JITServer instance it connects to must use the same (compatible) version. The easiest way to ensure this is using the same release on both JITServer JDK and application JDK.
* Deploys on Linux on x86-64, Linux on Power and Linux on Z 64-bit only. In the future other platforms may be supported as well.
* Supports Java 8 and Java 11. Other Java versions may be supported in future releases.
