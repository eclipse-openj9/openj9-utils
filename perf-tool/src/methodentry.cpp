// JNIEXPORT void JNICALL MethodEntry(jvmtiEnv *jvmtiEnv,
//             JNIEnv* jni_env,
//             jthread thread,
//             jmethodID method) {
//     /*jvmtiError err;
//     json j;
//     char *methodName;
//     err = jvmtiEnv->GetMethodName(method, &methodName, NULL, NULL);
//     if (err == JVMTI_ERROR_NONE && strcmp(methodName, "doBatch") == 0) {
//         j["methodName"] = methodName;
//         std::string s = j.dump();

//         printf("\n%s\n", s.c_str());
//     }*/
//     jvmtiError error;

//     error = (*jvmti)->GetMethodName(jvmti, method, &name_ptr, &signature_ptr, &generic_ptr);

//     check_jvmti_errors(jvmti, error, "Unable to get the method name");

//     printf("Entered method %s\n", name_ptr);

// }