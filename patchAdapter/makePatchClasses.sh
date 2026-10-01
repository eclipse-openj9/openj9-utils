######################
#
# Makes the set of patch classes
# necesesary due to the fact that
# maven will not allow 2 variants of
# same class, which is necessary to have
# a patch set and original set for testing
#
# If you expand the testset, add to this file!
#
# To cleanup the products of this, after testing, use:
#     find resources/testexamplespatch/methodadditionpolymorphic/patch/testexamples/methodadditionpolymorphic/ -name "*.class" | xargs  rm
#
#
# IF THIS SCRIPT FAILS PARTWAY THROUGH:
# you should fix the issue then run the rest of the steps copy/paste style
# if you restart the whole script you might erase the original copies of test setup classes
# 
######################

mkdir TEMP
echo "----------------------------------------"
echo "Copying original versions of source files into TEMP"
cp -r src/main/java/testexamples TEMP/
echo "----------------------------------------"

echo "----------------------------------------"
echo "Copying patch sets into build location"
cp resources/testexamplespatch/fieldaddition/patch/testexamples/fieldaddition/*.java src/main/java/testexamples/fieldaddition/
cp resources/testexamplespatch/methodremoval/patch/testexamples/methodremoval/*.java src/main/java/testexamples/methodremoval/
cp resources/testexamplespatch/nochange/patch/testexamples/nochange/*.java src/main/java/testexamples/nochange/
cp resources/testexamplespatch/methodadditionmonomorphic/patch/testexamples/methodadditionmonomorphic/*.java src/main/java/testexamples/methodadditionmonomorphic
cp resources/testexamplespatch/methodadditionpolymorphic/patch/testexamples/methodadditionpolymorphic/*.java src/main/java/testexamples/methodadditionpolymorphic
echo "----------------------------------------"

echo "----------------------------------------"
echo "Compiling!"
mvn compile
echo "----------------------------------------"

if [[ "$?" -ne 0 ]] ; then
    echo 'Could not compile test setup'; exit $rc

else
    
    echo "----------------------------------------"
    echo "Copying patch classfiles into expected directory for patch adapter testing"
    cp target/classes/testexamples/fieldaddition/* resources/testexamplespatch/fieldaddition/patch/testexamples/fieldaddition/
    cp target/classes/testexamples/methodremoval/* resources/testexamplespatch/methodremoval/patch/testexamples/methodremoval/
    cp target/classes/testexamples/nochange/* resources/testexamplespatch/nochange/patch/testexamples/nochange/
    cp target/classes/testexamples/methodadditionmonomorphic/* resources/testexamplespatch/methodadditionmonomorphic/patch/testexamples/methodadditionmonomorphic/
    cp target/classes/testexamples/methodadditionpolymorphic/* resources/testexamplespatch/methodadditionpolymorphic/patch/testexamples/methodadditionpolymorphic/
    echo "----------------------------------------"

    echo "----------------------------------------"
    echo "Replacing original versions of source files"
    cp -r TEMP/testexamples/ src/main/java/testexamples/
    echo "----------------------------------------"


    echo "----------------------------------------"
    echo "Cleaning up!"
    rm -rf TEMP
    echo "----------------------------------------"

fi
