# Create your own JITServer Operator 

### Requirements
- `operator-sdk`
- `kubectl`
- `helm`
- A running kubernetes cluster

### Steps

- Add OpenJ9 JITServer helm chart repository to the Helm client:
  
  ```
  helm repo add openj9 https://raw.githubusercontent.com/eclipse/openj9-utils/master/helm-chart/
  helm repo update
  helm search repo openj9-jitserver-chart
  ```
  
- Use the CLI to create a new Helm-based jitserver-operator project:
  
  ```
  mkdir jitserver-operator
  cd jitserver-operator
  operator-sdk init --plugins helm --helm-chart openj9/openj9-jitserver-chart --domain openj9jitserver
  ```
  
- Build and push your operator’s image:
  
  ```
  make docker-build docker-push IMG="<your-docker-hub-user-namespace>/jitserver-operator:v0.0.1"
  ```
  
- Run the operator as a Deployment inside the cluster:
  
  ```
  make deploy IMG="<your-docker-hub-user-namespace>/jitserver-operator:v0.0.1"
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

- Update the sample JITServer Custom Resource manifest at `config/samples/charts_v1alpha1_openj9jitserverchart.yaml` to set your desired values.

- Create a JITServer Custom Resource:
  
  ```
  kubectl apply -f config/samples/charts_v1alpha1_openj9jitserverchart.yaml
  ```

- Ensure that the JITServer operator creates the deployment for the Custom Resource:
  
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

