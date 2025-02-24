#include <array>
#include <cstring>
#include <format>
#include <print>
#include <stdexcept>
#include <string>

#include "jvm_attachment.hpp"

JvmAttachment::JvmAttachment():
    jvm(nullptr),
    jni_env(nullptr),
    attachment(nullptr) {
  std::array<JavaVMOption, 0> options = {};

  // JVM argument configuration
  JavaVMInitArgs vm_args = {};
  memset(&vm_args, 0, sizeof(vm_args));

  vm_args.ignoreUnrecognized = false;
  vm_args.nOptions = options.size();
  vm_args.options = options.data();
  vm_args.version = JNI_VERSION_10;

  // JVM Initialization
  void* jni_env_casted = static_cast<void*>(jni_env);
  JNI_CreateJavaVM(&jvm, &jni_env_casted, &vm_args);
  jni_env = static_cast<JNIEnv*>(jni_env_casted);

  // Bail if our JVM or JNI environment aren't loaded properly as
  // we won't be able to do anything in these instances
  if (!jvm) {
    throw std::runtime_error("Creation of JVM instance failed");
  }

  if (!jni_env) {
    throw std::runtime_error("JNI environment is unpopulated");
  }
}

JvmAttachment::~JvmAttachment() {
  if (jvm) {
    jvm->DestroyJavaVM();
  }
}

void JvmAttachment::abort_if_jvm_exception() const {
  if (jni_env->ExceptionCheck() == JNI_TRUE) {
    // Get the exception object, necessary method(s)
    jthrowable ex = jni_env->ExceptionOccurred();
    jclass ex_class = load_class("java/lang/Exception");
    jmethodID get_message = load_method(ex_class, "getMessage", "()Ljava/lang/String;");

    // Get calls to obtain exception type metadata
    jclass ex_type = jni_env->GetObjectClass(ex);
    jclass klass = load_class("java/lang/Class");
    jmethodID simple_name = load_method(klass, "getSimpleName", "()Ljava/lang/String;");

    // Access exception type
    jstring ex_type_name = static_cast<jstring>(
      jni_env->CallObjectMethod(ex_type, simple_name)
    );

    const char* ex_type_name_chars = jni_env->GetStringUTFChars(ex_type_name, nullptr);

    // Access exception message
    jstring ex_message = static_cast<jstring>(jni_env->CallObjectMethod(ex, get_message));
    const char* ex_message_chars = jni_env->GetStringUTFChars(ex_message, nullptr);      

    // Format our own exception message
    auto exception_msg = std::format(
        "{}: {}\n\nException occurred while calling native JVM method",
        ex_type_name_chars,
        ex_message_chars
    );

    // Cleanup
    jni_env->ReleaseStringUTFChars(ex_type_name, ex_type_name_chars);
    jni_env->ReleaseStringUTFChars(ex_message, ex_message_chars);
    jni_env->DeleteLocalRef(ex_message);
    jni_env->DeleteLocalRef(ex);

    // Clear for exceptions elsewhere, then throw for handling further up the application
    jni_env->ExceptionClear();
    throw std::runtime_error(exception_msg);
  }
}

jclass JvmAttachment::load_class(std::string_view class_name) const {
  jclass found_class = jni_env->FindClass(class_name.data());

  if (!found_class) {
    throw std::runtime_error(
      std::format("Failed to load requested class: {0}", class_name)
    );
  }

  return found_class;
}

jmethodID JvmAttachment::load_method(
    jclass class_ref,
    std::string_view method_name,
    std::string_view signature) const {
  jmethodID method = jni_env->GetMethodID(class_ref, method_name.data(), signature.data());

  if (!method) {
    throw std::runtime_error(
      std::format("Failed to load requested method: {0} [{1}]", method_name, signature)
    );
  }
  
  return method;
}

jmethodID JvmAttachment::load_static_method(
    jclass class_ref,
    std::string_view method_name,
    std::string_view signature) const {
    jmethodID method = jni_env->GetStaticMethodID(
      class_ref,
      method_name.data(),
      signature.data()
    );

    if (!method) {
      throw std::runtime_error(
        std::format("Failed to load requested static method: {0} [{1}]", method_name, signature)
      );
    }

    return method;
}

void JvmAttachment::attach(pid_t java_proc) {
  // Prevent duplicate attachment
  if (attachment) {
    throw std::runtime_error(
      "Attempted to perform an attachment when already targeting another JVM instance"
    );
  }

  // Load appropriate data from JNI
  jclass virtual_machine = load_class("com/sun/tools/attach/VirtualMachine");
  jmethodID attach = load_static_method(
    virtual_machine,
    "attach",
    "(Ljava/lang/String;)Lcom/sun/tools/attach/VirtualMachine;"
  );

  // Create args to the JVM function
  auto pid_str = std::to_string(java_proc);
  jstring jvm_pid_str = jni_env->NewStringUTF(pid_str.c_str());

  // Call out to the JVM thread and cleanup after ourselves
  jobject attachment_object = jni_env->CallStaticObjectMethod(
    virtual_machine,
    attach,
    jvm_pid_str
  );

  abort_if_jvm_exception();
  jni_env->DeleteLocalRef(jvm_pid_str);

  // Set our attachment as a global ref to enforce that we have to delete it
  attachment = jni_env->NewGlobalRef(attachment_object);
  jni_env->DeleteLocalRef(attachment_object);
}

void JvmAttachment::detach() {
  // NOOP if we're not pointing to anything valid
  if (!attachment) {
    return;
  }

  // Load appropriate data form JNI
  jclass virtual_machine = load_class("sun/tools/attach/VirtualMachineImpl");
  jmethodID detach = load_method(virtual_machine, "detach", "()V");

  // Call out to the JVM thread, then remove the global reference we're keeping
  jni_env->CallVoidMethod(attachment, detach);
  abort_if_jvm_exception();
  jni_env->DeleteGlobalRef(attachment);

  // Ensure we're pointing to invalid data now
  attachment = nullptr;
}

void JvmAttachment::load_agent(std::string_view dynamic_library) const {
  // Get the method for loading a native library
  jclass virtual_machine = load_class("com/sun/tools/attach/VirtualMachine");
  jmethodID agent_load = load_method(
    virtual_machine,
    "loadAgentLibrary",
    "(Ljava/lang/String;)V"
  );

  // Insert the agent library into the remote JVM's live phase
  jstring library_path = jni_env->NewStringUTF(dynamic_library.data());
  jni_env->CallVoidMethod(attachment, agent_load, library_path);
  abort_if_jvm_exception();
  jni_env->DeleteLocalRef(library_path);
}
