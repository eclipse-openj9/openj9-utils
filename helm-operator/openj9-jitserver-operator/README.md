# JITServer Operator 

## Installation and deployment

### Requirements
- `kubectl` installed
- A running kubernetes cluster

### Installation
You can install the JITServer Operator into a Kubernetes cluster via `kubectl` commands.

- Deploy the operator:
  
  ```
  kubectl apply -f deploy.yaml
  ```

- Verify the operator is running:
  
  ```
  kubectl get deployment -n jitserver-operator-system | grep jitserver-operator
  
  jitserver-operator-controller-manager   1/1     1            1           11s
  ```
  
  ```
  kubectl get pods -n jitserver-operator-system | grep jitserver-operator
  
  jitserver-operator-controller-manager-7dc5df6d68-g62kr   2/2     Running   0          48s
  ```

- Deploy the Custom Resource:
  
  ```
  kubectl apply -f charts_v1alpha1_openj9jitserverchart.yaml
  ```

- Verify the Custom Resource is running:
  
  ```
  kubectl get deployment | grep openj9jitserverchart
  
  openj9jitserverchart-sample-openj9-jitserver-chart   1/1     1            1           13s
  ```
  
  ```
  kubectl get pods | grep openj9jitserverchart
  
  openj9jitserverchart-sample-openj9-jitserver-chart-5d5696fcz626   1/1     Running   0          54s
  ```

- Issue a sample query to the Custom Resource:
  
  ```
  export POD_NAME=$(kubectl get pod --namespace default -o jsonpath="{..metadata.name}" | grep openj9jitserverchart-sample-openj9-jitserver-chart)
  kubectl exec $POD_NAME -i -t -- java -version
  
  openjdk version "1.8.0_322"
  IBM Semeru Runtime Open Edition (build 1.8.0_322-b06)
  Eclipse OpenJ9 VM (build openj9-0.30.0, JRE 1.8.0 Linux amd64-64-Bit Compressed References 20220128_306 (JIT enabled, AOT enabled)
  OpenJ9   - 9dccbe076
  OMR      - dac962a28
  JCL      - c1d9a7af7c based on jdk8u322-b06)
  ```

### Uninstallation
- Clean up the Custom Resource:
  ```
  kubectl delete -f charts_v1alpha1_openj9jitserverchart.yaml
  ```
  
- Delete the operator:
  ```
  kubectl delete -f deploy.yaml
  ```

