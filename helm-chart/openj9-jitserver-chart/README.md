<!--
Copyright (c) 2020, 2022 IBM Corp. and others

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

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
-->

# Eclipse OpenJ9 JITServer Helm Chart

## Introduction.
The OpenJ9 JITServer Helm Chart allows you to deploy the JITServer technology into Kubernetes or OpenShift clusters and makes it possible for your Java applications to take advantage of it. JITServer is a technology that offers relief from the negative side-effects of the JIT compilation which are mostly felt in short lived applications running in resource constrained environments. The idea behind the technology is to decouple the JIT compiler from the JVM, containerize it and let it run as-a-service, in the cloud, where it can be managed intelligently by an off-the-shelf container orchestrator like K8s. JITServer is available as a fully supported feature of the OpenJ9 JVM and is supported on Linux on x86-64, Linux on Power and Linux on Z running Java8, Java11 or Java17. For the most up to date information, please check supported configurations at [JITServer technology introduction](https://www.eclipse.org/openj9/docs/jitserver/).

## Getting started with OpenJ9 JITServer Technology.
JITServer technology can be used for any Java application running on release 0.18.0 (or newer) of Open9 on supported Java versions and Linux platforms. You can make use of JITServer technology following the steps below:

1. Install the JITServer using Helm Chart
2. Enable your application to use JITServer

### 1. Install the JITServer using Helm Chart.
#### a. Verify the application JDK version and select the appropriate JITServer image.
**Important:** The client JVM and the JITServer instance it connects to must use the same (compatible) version. The easiest way to ensure this is using the same release for the JITServer JDK as used by the application.

Inside your Java application image, run `java -version` to check the JDK release version. In the example below, the `0.32.0` release of Java 11 JDK on `Linux amd64` architecture is used as shown in `build openj9-0.32.0, JRE 11 Linux amd64-64-Bit`.
```bash
$ java -version
openjdk version "11.0.15" 2022-04-19
IBM Semeru Runtime Open Edition 11.0.15.0 (build 11.0.15+10)
Eclipse OpenJ9 VM 11.0.15.0 (build openj9-0.32.0, JRE 11 Linux amd64-64-Bit Compressed References 20220422_425 (JIT enabled, AOT enabled)
OpenJ9   - 9a84ec34e
OMR      - ab24b6666
JCL      - b7b5b42ea6 based on jdk-11.0.15+10)
```

Find the corresponding image from [IBM Semeru Runtimes Docker Hub](https://hub.docker.com/_/ibm-semeru-runtimes) through keyword search of architecture, Java version and release. In this example, `0.32.0` release of Java 11 is used in the application image, thus the corresponding image with tag is [`ibm-semeru-runtimes:open-11.0.15_10-jre`](https://hub.docker.com/layers/ibm-semeru-runtimes/library/ibm-semeru-runtimes/open-11.0.15_10-jre/images/sha256-a7f2c008c986dd45e22c764333a7182ab0872d80d52f3e5e8d1195b3a3a0108b?context=explore). The image tag `open-11.0.15_10-jre` will be used in the following step. For convenience, [Table 1](#table-1-image-tags-from-ibm-semeru-runtimes-repository-in-docker-hub) below lists all IBM Semeru Runtimes tags per OpenJ9 release and Java version.

#### Table 1. Image tags from IBM Semeru Runtimes repository in Docker Hub
OpenJ9 Release | Java 8             | Java 11              | Java 17            | Java 21
---------------|--------------------|----------------------|--------------------|--------------
0.27.0         | open-8u302-b08-jre | open-11.0.12_7-jre   |                    |
0.29.0         | open-8u312-b07-jre | open-11.0.13_8-jre   |                    |
0.29.1         |                    |                      | open-17.0.1_12-jre |
0.30.0         | open-8u322-b06-jre | open-11.0.14_8-jre   | open-17.0.2_8-jre  |
0.30.1         |                    | open-11.0.14.1_1-jre |                    |
0.32.0         | open-8u332-b09-jre | open-11.0.15_10-jre  | open-17.0.3_7-jre  |
0.33.1         | open-8u345-b01-jre | open-11.0.16.1_1-jre | open-17.0.4.1_1-jre|
0.35.0         | open-8u352-b08-jre | open-11.0.17_8-jre   | open-17.0.5_8-jre  |
0.36.0         | open-8u362-b09-jre | open-11.0.18_10-jre  | open-17.0.6_10-jre |
0.38.0         | open-8u372-b07-jre | open-11.0.19_7-jre   | open-17.0.7_7-jre  |
0.40.0         | open-8u382-b05-jre | open-11.0.20_8-jre   | open-17.0.8_7-jre  |
0.41.0         | open-8u392-b08-jre | open-11.0.21_9-jre   | open-17.0.9_9-jre  |
0.43.0         | open-8u402-b06-jre | open-11.0.22_7-jre   | open-17.0.10_7-jre |
0.44.0         | open-8u412-b08-jre | open-11.0.23_9-jre   | open-17.0.11_9-jre | open-21.0.3_9-jre
0.46.1         | open-8u422-b05-jre | open-11.0.24_8-jre   | open-17.0.12_7-jre | open-21.0.4_7-jre
0.48.0         | open-8u432-b06-jre | open-11.0.25_9-jre   | open-17.0.13_11-jre| open-21.0.5_11-jre
0.49.0         | open-8u442-b06-jre | open-11.0.26_4-jre   | open-17.0.14_7-jre | open-21.0.6_7-jre


**Note:** up until release 0.26.0, OpenJ9 JVM images could be found in the [AdoptOpenJDK](https://hub.docker.com/_/adoptopenjdk) repo of Docker Hub. Starting with release 0.27.0, OpenJ9 JVM images can be pulled from their new repo, [IBM Semeru Runtimes](https://hub.docker.com/_/ibm-semeru-runtimes) in Docker Hub.


#### b. Install the JITServer using the selected image and the Helm Chart.
Add OpenJ9 JITServer helm chart repository to the Helm client.
``` bash
helm repo add openj9 https://raw.githubusercontent.com/eclipse/openj9-utils/master/helm-chart/
helm repo update
helm search repo openj9-jitserver-chart
```

Install OpenJ9 JITServer helm chart by following below command.
``` bash
helm install jitserver-release openj9-jitserver-chart
```

This chart deploys a JITServer instance with latest OpenJ9 Java 8 release on x86-64 platform by default. If your application uses a different version of Java, please include the option `--set image.tag="REPLACE_IMAGE_TAG"` in above command which overrides below section in `Values.yaml`.
```yaml
image:
  tag: open-8u332-b09-jre
```

Currently, JITServer is supported on Java versions 8, 11, 17 and 21.

### 2. Enable your application to use JITServer.
Once the JITServer is deployed, you can enable your application to use JITServer by adding the following JVM options as an environment variable into your Java application container.

Service names can be found by `kubectl get service`. `<serverport>` is set to `38400` by default.
``` bash
_JAVA_OPTIONS = "-XX:+UseJITServer -XX:JITServerAddress=<servicename> -XX:JITServerPort=<serverport>"
```

On the JITServer side, `<servicename>` is set automatically by helm, and you may change `<serverport>` by including the option `--set service.port="REPLACE_PORT_NUMBER"`. This option overrides below section in `Values.yaml`.
```yaml
service:
  port: 38400
```

Setting the `_JAVA_OPTIONS` environment variable on a new or an already in production application depends on your application build/deployment pipeline. For more detailed instructions on how to redeploy your application with the above environment variable changes, please read through [this tutorial](./enable-jitserver.md). The tutorial shows how to enable JITServer technology for an example OpenLiberty Java application.

## JITServer considerations and platform support
### How to select a platform and a Java version
Platforms are handled by multi-arch images from AdoptOpenJDK or IBM Semeru Runrtimes repositories in Docker Hub, thus platform specific JITServer is deployed based on which platform the install is being done on. The JITServer Java version has to exactly match the application Java version. For example, Java 8 applications need Java 8 JITServer running on the exact same JDK version. (See details above)

If your application uses a different version of Java, please include the options `--set image.repository="REPLACE_IMAGE_REPO"` and `--set image.tag="REPLACE_IMAGE_TAG"` in the `helm install` command.

Alternatively, if you have a copy of the helm chart, then, in order to select a specific Java version, you can edit `image.repository` and `image.tag` fields inside `values.yaml` to change the image location.

For example, this helm chart currently pulls AdoptOpenJDK OpenJ9 Java 8 `0.24.0` release images (`adoptopenjdk:8u282-b08-jdk-openj9-0.24.0`).
``` yaml
image:
  repository: adoptopenjdk
  tag: 8u282-b08-jdk-openj9-0.24.0
  pullPolicy: IfNotPresent
```

Should you want to deploy IBM Semeru Runtimes OpenJ9 Java 11 `0.32.0` release images, change `image` fields inside `Values.yaml` as shown below.
``` yaml
image:
  repository: ibm-semeru-runtimes
  tag: open-11.0.15_10-jre
  pullPolicy: IfNotPresent
```

The complete list of available images and tags can be found at [AdoptOpenJDK Docker Hub](https://hub.docker.com/_/adoptopenjdk?tab=tags) and [IBM Semeru Runtimes Docker Hub](https://hub.docker.com/_/ibm-semeru-runtimes?tab=tags). A short list of tags from the `docker.io/ibm-semeru-runtimes` repository is included in the [Table 1](#table-1-image-tags-from-ibm-semeru-runtimes-repository-in-docker-hub).


### JITServer and application deployment topology considerations
One or more application(s) can connect to one JITServer instance as long as they are running the same JDK version and enough resources are allocated for JITServer. However, a single application should not be allowed to connect to multiple JITServer instances.

JITServer can be deployed either on the same node or on a different node from the Java application. There are benefits from both methods.

* Deploy JITServer on the same node as the Java application
If the node that Java application runs on has enough resources, deploying JITServer on the same node is recommended since this reduces networking overheads and maximizes JITServer performance. Note that this may not always be possible, if you want to connect multiple JVM clients to a single JITServer instance.

* Deploy JITServer on a different node as the Java application
If the node that Java application runs on does not have enough CPU or memory resources, JITServer can be deployed on a different node.

You may specify the node to deploy on by editing `affinity.nodeAffinity.requiredDuringSchedulingIgnoredDuringExecution[].values[]` in `values.yaml`. Node names can be found by `kubectl get nodes`.
``` yaml
affinity:
  nodeAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - "example-node-name"
```

## Application and JITServer life cycle management

### Verifying if the JITServer is deployed
``` bash
$ helm status jitserver-release
NAME: jitserver-release
LAST DEPLOYED: Wed Nov 18 18:41:07 2020
NAMESPACE: helm-chart-test
STATUS: deployed
REVISION: 1
NOTES:
Welcome to OpenJ9 JITServer Helm Chart, the application has been deployed successfully.
```

``` bash
$ helm test jitserver-release
Pod jitserver-release-openj9-jitserver-chart-test-connection pending
Pod jitserver-release-openj9-jitserver-chart-test-connection succeeded
NAME: jitserver-release
LAST DEPLOYED: Wed Nov 18 19:19:46 2020
NAMESPACE: helm-chart-test
STATUS: deployed
REVISION: 1
TEST SUITE:     jitserver-release-openj9-jitserver-chart-test-connection
Last Started:   Wed Nov 18 19:41:14 2020
Last Completed: Wed Nov 18 19:41:26 2020
Phase:          Succeeded
NOTES:
Welcome to OpenJ9 JITServer Helm Chart, the application has been deployed successfully.
```

### Upgrade JITServer JDK upon application of a JDK upgrade.
The JITServer instance must be compatible with the application JDK it serves. The easiest way to ensure this is using the same OpenJ9 release on both JITServer JDK and application JDK. Please follow [Step 1a](./README.md#a-verify-the-application-jdk-version-and-select-the-appropriate-jitserver-image) to find out the application JDK build version before proceeding.

#### Scenario 1: All applications are being upgraded to a newer JDK version.
If all applications are being upgraded to a newer version of OpenJ9 JDK, you may perform a `helm upgrade` to upgrade the JITServer instances to be compatible with your applications. In the command below, replace `REPLACE_IMAGE_TAG` with the desired image tag of the JITServer to be deployed.
```bash
helm upgrade --set image.tag="REPLACE_IMAGE_TAG" jitserver-release openj9-jitserver-chart
```

#### Scenario 2: Only some applications are being upgraded to a newer version of JDK.
If some applications are upgraded to a newer OpenJ9 JDK version while others are still using the older version of OpenJ9 JDK, you need to keep the existing JITServer and deploy another JITServer instance that is compatible with the newer version of JDK. Replace `REPLACE_IMAGE_TAG` with the desired image tag to be deployed as a JITServer instance.
```bash
helm install --set image.tag="REPLACE_IMAGE_TAG" jitserver-release-latest openj9-jitserver-chart
```
Note that the new versions of the applications must also update the value of the `-XX:JITServerAddress=<servicename>` option to reflect the endpoint of the new JITServer instance.
When all applications completed the upgrade and the older versions of the applications are retired, you are safe to delete the older version of JITServer.
``` bash
helm delete jitserver-release
```

### Removing the JITServer
Avoid removing the JITServer instance before applications terminate. If the memory limit for the container running the application JDK has been set based on the assumption that JITServer handles all JIT compilations, the application may experience a native out-of-memory scenario and be terminated.
Once all applications are retired, you can safely remove the JITServer instance.
``` bash
helm delete jitserver-release
```

## Resources Required
### System
The following are recommended minimum values for CPU and memory resources at JITServer, based on the assumption of a single application connecting to the JITServer instance:
* CPU Requested : 1 CPU
* Memory Requested : 512 MB
If multiple applications connect concurrently to one JITServer instance, multiply the above values accordingly. Please note that, if multiple applications connect to the JITServer in a staggered fashion (i.e. with some delay between applications), then you could lower the above requirements.

### Storage
* Currently no storage or volume needed.

## Chart Details
This chart installs one JITServer instance that is ready to serve applications. It contains the following two K8s objects:
* One `deployment` running an IBM Semeru Runtimes OpenJ9 image
* One `service` that is JITServer technology endpoint

## Prerequisites
### Red Hat OpenShift SecurityContextConstraints Requirements
This chart does not require SecurityContextConstraints to be bound to the target namespace prior to installation.

### PodSecurityPolicy Requirements
This chart does not require PodSecurityPolicy to be bound to the target namespace prior to installation.

### PodDisruptionBudgets Requirement
This chart does not require PodDisruptionBudgets to be bound to the target namespace prior to installation.

## Documentation
* OpenJ9 website: https://www.eclipse.org/openj9/index.html
* OpenJ9 JITServer technology: https://www.eclipse.org/openj9/docs/jitserver/
* OpenJ9 repository: https://github.com/eclipse/openj9
* OpenJ9 docs: https://github.com/eclipse/openj9-docs

## Limitations
* Deploys on Linux on x86-64, Linux on Power and Linux on Z 64-bit only. In the future other platforms may be supported as well.
* Supports Java 8, Java 11, Java17 and Java21 only. For the most up to date information, please check the [JITServer technology introduction](https://www.eclipse.org/openj9/docs/jitserver/).
