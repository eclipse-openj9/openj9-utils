#ifndef OBJECTALLOC_H_
#define OBJECTALLOC_H_

JNIEXPORT void JNICALL VMObjectAlloc(jvmtiEnv *jvmtiEnv, 
                        JNIEnv* env, 
                        jthread thread, 
                        jobject object, 
                        jclass object_klass, 
                        jlong size);


#endif /* OBJECTALLOC_H_ */ 