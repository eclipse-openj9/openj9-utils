# OpenJ9 JIT Debug Agent

This tool automatically obtains a limited JIT trace log for a miscompiled method. It works by using preexistence
and a hooked internal API to repeatedly run a test by sequentially reverting JIT methods to be executed in the interpreter.
By repeatedly executing a test in a controlled environment we are able to devertmine which JIT method needs to be interpreted
for the test to start passing. This is how the tool determines which JIT method _may_ be responsible for the test case failure.

Once the JIT method is identified the tool performs a `lastOptIndex` search by recompiling the JIT method at different
optimization levels. Once again, when the test starts passing we have determined the minimal `lastOptIndex` which causes the
failure. At that point the tool gather a _"good"_ and _"bad"_ JIT trace log with the optimization included and excluded for
JIT developers to investigate.

# When to use this tool

This tool should be used for highly intermittent JIT defects from automated testing environments where we are either not able
to determine the JIT method responsible for the failure, or we are unable to trace the suspect JIT method due to a Heisenbug.

# How to use this tool

1. Using maven and running the command `$ mvn install` in this directory will package the agent into a jar file, you will find it in the `target` directory. 
2. add the java agent option to your `java` execution which would look like this: `-javaagent: jit-debug-agent-1.0.jar`
3. Run the test by forcing preexistence.

    Currently the tool is not robust enough to enable this automatically, so we have to force every JIT method compilation to use
    preexistence. This is to ensure that we have a _revert to interpreter_ stub inserted in the preprologue of every method.
    Inserting this stub happens at binary encoding and should not affect the semantics of the method itself.

    ```
    java -Xdump:none -Xnoaot -Xcheck:jni -Xjit:forceUsePreexistence -javaagent: jit-debug-agent-1.0.jar <test>