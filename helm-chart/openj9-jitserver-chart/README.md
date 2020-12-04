<!--
Copyright (c) 2020, 2020 IBM Corp. and others

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

# Eclipse OpenJ9 JITServer Helm Chart

## Introduction.
The OpenJ9 JITServer Helm Chart allows you to deploy, manage JITServer technology into Kubernetes or OpenShift clusters and make it possible for your Java applications to take advantage of it. JITServer is a technology that offers relief from the negative side-effects of the JIT compilation which are mostly felt in short lived applications running in resource constrained environments. The idea behind the technology is to decouple the JIT compiler from the JVM, containerize it and let it run as-a-service, in the cloud, where it can be managed intelligently by an of-the-shelf container orchestrator like k8s. JITServer is available as a "tech preview" in the OpenJ9 JVM and is supported on Linux on x86-64, Linux on Power and Linux on Z running Java8 or Java11

## Getting started with OpenJ9 JITServer Technology.
JITServer technology can be used for any Java application running on the latest release of OpenJ9 on supported Java versions and Linux platforms. You can make use of JITServer technology following the steps below:

1. Install the JITServer using Helm Chart
2. Enable your application to use JITServer

### 1. Install the JITServer using Helm Chart.
#### a. Verify the application JDK version and select the appropriate JITServer image.
Important: The client JVM and the JITServer instance it connects to must use the same (compatible) version. The easiest way to ensure this is using the same release on both JITServer JDK and application JDK.

Inside your Java application image, run `java -version` to check the JDK release version. In the example below, the `0.21.0` release of Java 11 JDK on `Linux amd64` architecture is used as shown in `build openj9-0.21.0, JRE 11 Linux amd64-64-Bit`. 
```bash
$ java -version
openjdk version "11.0.8" 2020-07-14
OpenJDK Runtime Environment AdoptOpenJDK (build 11.0.8+10)
Eclipse OpenJ9 VM AdoptOpenJDK (build openj9-0.21.0, JRE 11 Linux amd64-64-Bit Compressed References 20200715_697 (JIT enabled, AOT enabled)
OpenJ9   - 34cf4c075
OMR      - 113e54219
JCL      - 95bb504fbb based on jdk-11.0.8+10)
```

Find the corresponding image from [AdoptOpenJDK Docker Hub](https://hub.docker.com/_/adoptopenjdk) through keywords search of architecture, Java version and release. In this example, `0.21.0` release of Java 11 is used in the application image, thus the corresponding image tag is [`11.0.8_10-jdk-openj9-0.21.0`](https://hub.docker.com/layers/adoptopenjdk/library/adoptopenjdk/11.0.8_10-jdk-openj9-0.21.0/images/sha256-996bb5bca1ae3fbab7bcd6ed4b19776eb1e176727024ce8866a75536137949b7?context=explore). The image tag will be used in the following step.

#### b. Install the JITServer using the selected image and the Helm Chart.
Add OpenJ9 JITServer helm chart repository to the Helm client.
``` bash
helm repo add openj9 https://<updatewhenweknowtheurl>stable/openj9/repo/index.yaml
helm repo update
helm search repo openj9-jitserver-chart
```

Install OpenJ9 JITServer helm chart by following below command.
``` bash
helm install jitserver-release openj9-jitserver-chart
```

This chart deploys a JITServer instance with latest OpenJ9 Java 8 release on x86 platform by default. If your application uses a different version of Java, please include the option `--set image.tag="REPLACE_IMAGE_TAG"` in above command which overrides below section in `Values.yaml`. 
```yaml
image:
  tag: 8-openj9
```

Java versions 8 and 11 are currently supported on x86, POWER and Z platforms.

### 2. Enable your application to use JITServer.
Once the JITServer is deployed, you can enable your application to use JITServer by adding the following JVM option as an environment variable into your Java application container.

Service names can be found by `kubectl get service`, `<serverport>` is set to `38400` by default. 
``` bash
JAVA_OPTIONS = "-XX:+UseJITServer -XX:JITServerAddress=<servicename> -XX:JITServerPort=<serverport>"
```

`<servicename>` is set automatically by helm, and you may change `<serverport>` by including option `--set service.port="REPLACE_PORT_NUMBER"`. This option overrides below section in `Values.yaml`. 
```yaml
service:
  port: 38400
```

Setting the `JAVA_OPTIONS` environment variable on a new or an already in production application depends on your application build/deployment pipeline. For more detailed instructions on how to redeploy your application with the above environment variable changes, please read through [this tutorial](./enable-jitserver.md). The tutorial shows how to enable JITServer technology for an example OpenLiberty Java application.

## JITServer considerations and platform support
### How to select a platform and a Java version 
Platforms are handled by multi-arch images from AdoptOpenJDK docker hub, thus platform specific JITServer is deployed based on which platform the install is being done on. The JITServer Java version has to match the application Java version. For example, Java 8 applications need Java 8 JITServer.

If your application uses a different version of Java, please include the option `--set image.tag="REPLACE_IMAGE_TAG"` in the `helm install` command which overrides below section in `Values.yaml`.

Alternatively, if you have a copy of the helm chart then, in order to select a specific java version, you can edit `image.repository` and `image.tag` fields inside `Values.yaml` to change the image location.

For example, this helm chart pulls `adoptopenjdk:8-openj9` image and runs `jitserver`.
``` yaml
image:
  repository: adoptopenjdk
  tag: 8-openj9
  pullPolicy: Always
  command: ["jitserver"]
```

Should you want to deploy AdoptOpenJDK Java 11 images, change `image` fields inside `Values.yaml` as shown below.
``` yaml
image:
  repository: adoptopenjdk
  tag: 11-openj9
  pullPolicy: Always
  command: ["jitserver"]
```

The complete list of avaliable images and tags can be found at [AdoptOpenJDK Docker Hub](https://hub.docker.com/_/adoptopenjdk?tab=tags).

### JITServer and application deployment topology considerations
One or more application(s) can connect to one JITServer instance as long as they are running the same JDK version and enough resources are allocated for JITServer. However, a single application should not be allowed to connect to multiple JITServer instances. 

JITServer can be deployed either on the same node or on a different node from the java application. There are benefits from both methods.

* Deploy JITServer on the same node as the java application
If the node that java application runs on has enough resources, deploying JITServer on the same node is recommended since this reduces networking overheads and maximizes JITServer performance. Note that this may not always be possible, if you want to connect multiple JVM clients to a single JITServer instance.

* Deploy JITServer on a different node as the java application
If the node that java application runs on does not have enough CPU or memory resources, JITServer can be deployed on a different node. 

You may specify the node to deploy on by editing `affinity.nodeAffinity.requiredDuringSchedulingIgnoredDuringExecution[].values[]` in `Values.yaml`. Node names can be found by `kubectl get nodes`.
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

### Upgrade JITServer JDK upon application JDK upgrade.
The JITServer instance must be compatible with the application JDK it serves. The easiest way to ensure this is using the same OpenJ9 release on both JITServer JDK and application JDK. Please follow [Step 1a](./README.md#a-verify-the-application-jdk-version-and-select-the-appropriate-jitserver-image) to find out application JDK build version before proceeding. 

#### Scenario 1: All applications are being upgraded to a newer JDK version.
If all applications are being upgraded to a newer version of OpenJ9 JDK, you may perform a `helm upgrade` to upgrade the JITServer instances to be compatible with your applications. Replace the `REPLACE_IMAGE_TAG` with the desired image tag of the JITServer to be deployed.
```bash
helm upgrade --set image.tag="REPLACE_IMAGE_TAG" jitserver-release openj9-jitserver-chart
```

#### Scenario 2: Only some applications are being upgraded to a newer version of JDK.
If some applications are upgraded to a newer OpenJ9 JDK version while others are still using the older version of OpenJ9 JDK, you need to keep the existing JITServer and deploy another JITServer instance that is compatible with newer version of JDK. Replace `REPLACE_IMAGE_TAG` with the desired image tag to be deployed as a JITServer instance. 
```bash
helm install --set image.tag="REPLACE_IMAGE_TAG" jitserver-release-latest openj9-jitserver-chart
```
When all applications completed upgrades, and the older versions of the application are retired, you are safe to delete the older version of JITServer.
``` bash
helm delete jitserver-release
```

### Removing the JITServer
Avoid removing the JITServer instance before applications terminate. There is a risk of application JVM has insufficient memory since it assumes JITServer handles the JIT Compilations.
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
* One `deployment` running AdoptOpenJDK OpenJ9 image
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
* Supports Java 8 and Java 11. Other Java versions may be supported in future releases.
