# Patch Adapter
  * a tool for adjusting software patches so that they can be used for hotfixing with a Java agent that uses the Instrumentation API
  * this project depends on [Soot](https://github.com/soot-oss/soot)!


## Motivation for a patch adapter:
  * Not all changes in a patch are supported when performing hotfixing! Change types that are not supported will cause Java Agents to throw UnsupportedOperation Exceptions. If these are not handled properly it can result in JVM shutdown, and even if they are handled, it means that the patch simply cannot be used, until it is adjusted. This tool does that, automatically!
  * currently follows [Instrumentation API](https://docs.oracle.com/en/java/javase/15/docs/api/java.instrument/java/lang/instrument/Instrumentation.html#redefineClasses(java.lang.instrument.ClassDefinition...)) constraints from `redefineClasses` method in JavaSE 15
  * specific class file changes supported by `redefineClasses` described in [jvmti docs](https://docs.oracle.com/en/java/javase/15/docs/specs/jvmti.html#RedefineClasses)

## How it works:
  * the tool runs in two phases, where each invokes a new run of Soot
    1) renames the set of original classes listed in x.originalclasses.out and places in to the directory specified by the `renamedOriginals` option
    2) Soot loads both the renamed originals and the classes in the patch, then :
	   * SemanticDiffer detects differences of interest between the versions
	   * SemanticDiffer passes points of interest to PatchAdapter to transform the redefinition classes
	   * a set of (possibly transformed) classes are emitted into the directory `adapterOutput`
  * x.originalclasses.out must contain names of all classes to be analysed by the patch adapter, this includes any containing references to added/removed fields/methods, patch adapter does not currently support discovery of locations (in non-specified classes) that require adjustment
  * patch class set must be located in directories, jar class loading is not currently supported
  
## How to build:
  * `mvn clean compile assembly:single` will build the application including dependencies


## How to run:
```
java -cp $cp differ.SemanticDiffer -cp cpplaceholder -w -renameDestination tempDirectory -finalDestination adapterOutput -redefcp redefDir -runRename [true/false] -mainClass mainClass -fullDir [true/false] -originalclasslist x.originalclasses.out Example
```
  * where:
     * $cp is the path to the SemanticDiffer package
     * cpplaceholder contains the following (in the listed order):
     	* originaldir: the directory of the original version of the class
	* rt.jar: a path to the jvm's rt
	* jce.jar: a path to the jvms crypto library
	* the directory of the patch/redefined version of the relevant classes
    * tempDirectory is the name that you would like to be used for the temporary directory that will contain a set of renamed classes that are output in the first invocation of Soot in the patch adapter
    * adapterOutput is the name that you would like to be used for the directory that is used for the final output of the patch adapter
    * redefDir is the directory of the patch/redefined version of the relevant classes
    * runrename controls whether the first invocation of Soot is run, i.e., whether the renamed set of classes will be output to the tempDirectory. If this step has already been performed for some patch, and those files are still present, then runRename can safely be set to false
    * mainClass is a (currently non) optional argument and can be used to set the "main class" that is used for the second invocation of Soot in the patch adapter. If the mainClass argument is omitted, then the first class in the originalclasses file will be used, however mainClass is currently used to set the package names when loading classes in the patch adapter specific run(s) of Soot. In the future a smarter approach would be to retain package names from x.originalclasses.out, or to have a loader discover classes in the patch directory, or to load from jars. 
    * fullDir is an optional argument that specifies whether you would like Soot to use the redefcp option path as the directory to consider all classes from. If set to true, in Soot's callgraph (CG) generation, all classes in redefcp are considered application classes and each of their methods are modelled as entry points to the application. If set to false, only (all) methods in the mainClass class will be used as entry points for the CG, which may be unrealistic, depending on the patch structure.
    * x.originalclasses.out is the file that contains the names of the classes that are relevant to the patch, and should be analysed by the patch adapter, both for diffing purposes and for fixing purposes
    * Example is any class, as a placeholder to get Soot up and running in a way that it is familiar with, however Example is included in this project, so for simplicity sake, Example can be used. Currently compiled into `target/classes/`, so anything that runs needs to keep that in mind, cp-wise. 

## Testing:
   * to setup the patch classes required for testing, first run `./makePatchClasses.sh`
   * then tests can be run with: `mvn test`
   * testing requires additional heap, currently set in `pom.xml`, minimum required is 1gb
   * to run only one test: `mvn -Dtest=ValidationTest#BTestRunFieldAddition test`
   * to expand the testset:
     1) create the **patch version** to use as a test class in `src/main/java/testexamples/<specific_testset_package_name>`
     2) get the classfile for that, i.e., `mvn compile`
     3) find the classfile for the new patch (`find . -name X.class`), then copy it and its source to `src/main/java/patch/testexamples/<specific_testset_package_name>'
     4) rename the package for the source in `src/main/java/testexamplespatch/<specific_testset_package_name>/patch/testexamples/<specific_testset_package_name>` to `patch.testexamples.<specific_testset_package_name>` so that when this recompiles when rebuilding the project, it does not conflict with original version of same class. Additionally they are each in a separate test dir in case the process_dir option of Soot is used during testing (set by useFullDir).
     5) now create the corresponding **original version** of that class in `src/main/java/testexamples/<specific_testset_package_name>`
     6) add corresponding tests for this added setup (see below for important detail)
     7) add to the `makePatchClasses.sh` script to assure that setup can occur correctly
   * tests are setup to run in alphabetical order, a surefire way to ensure that they are run sequentially. If they are run in parallel, Soot will share many values, as it uses several singletons to model the application and this could cause issues if it is done unintentionally
   * tests additionally must use the `TestSetup.testSetupRefresh` method to refresh all Soot settings, and pause to allow for that to commence, otherwise there will likely be overlapping data in Soot's model of the application, which can cause issues.