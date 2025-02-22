#include <cstring>
#include <stdexcept>
#include <string>

#include "jvm_attachment.hpp"

JvmAttachment::JvmAttachment():
    jvm(nullptr),
    jni_env(nullptr),
    virtual_machine(nullptr),
    virtual_machine_impl(nullptr),
    attach(nullptr),
    detach(nullptr) {
  JavaVMOption options[0];

  // JVM argument configuration
  JavaVMInitArgs vm_args = {};
  memset(&vm_args, 0, sizeof(vm_args));

  vm_args.ignoreUnrecognized = false;
  vm_args.nOptions = 0;
  vm_args.options = options;
  vm_args.version = JNI_VERSION_21;

  // JVM Initialization
  void* jni_env_casted = static_cast<void*>(jni_env);
  JNI_CreateJavaVM(&jvm, &jni_env_casted, &vm_args);
  jni_env = static_cast<JNIEnv*>(jni_env_casted);

  if (!jvm) {
    throw std::runtime_error("Creation of JVM instance failed");
  }

  if (!jni_env) {
    throw std::runtime_error("JNI environment is unpopulated");
  }

  // Obtain the appropriate class in the Attach APi
  virtual_machine = jni_env->FindClass("com/sun/tools/attach/VirtualMachine");

  if (!virtual_machine) {
    throw std::runtime_error("Unable to find the Sun Attach API classes");
  }

  // Load a handle to our attach method
  attach = jni_env->GetStaticMethodID(
    virtual_machine,
    "attach",
    "(Ljava/lang/String;)Lcom/sun/tools/attach/VirtualMachine;"
  );

  if (!attach) {
    throw std::runtime_error("Unable to load VirtualMachine attachment function");
  }

  // Obtain OpenJDK's implementation of the VirtualMachine class
  virtual_machine_impl = jni_env->FindClass("sun/tools/attach/VirtualMachineImpl");

  if (!virtual_machine_impl) {
    throw std::runtime_error("Unable to load OpenJDK Virtual Machine class implementation");
  }

  // Load a handle to our detach method
  detach = jni_env->GetMethodID(
    virtual_machine_impl,
    "detach",
    "()V"
  );

  if (!detach) {
    throw std::runtime_error(
      "Unable to find the `detach` method on the OpenJDK implementation"
    );
  }
}

JvmAttachment::~JvmAttachment() {
  if (jvm) {
    jvm->DestroyJavaVM();
  }
}

void JvmAttachment::attach_to_remote(pid_t java_proc) {
  // Create args to the JVM function
  auto pid_str = std::to_string(java_proc);
  jstring jvm_pid_str = jni_env->NewStringUTF(pid_str.c_str());

  // Attach to the remote VM
  jobject vm_obj = jni_env->CallStaticObjectMethod(virtual_machine, attach, jvm_pid_str);

  if (!vm_obj) {
    throw std::runtime_error("Attachment to remote JVM process failed");
  }

  if (jni_env->ExceptionCheck() == JNI_TRUE) {
    throw std::runtime_error("Exception occurred during call to VirtualMachine.attach()");
  }

  // TODO(Garrett): Dynamically load agent

  // Detach from the remote VM
  jni_env->CallVoidMethod(vm_obj, detach);

  if (jni_env->ExceptionCheck()) {
    throw std::runtime_error("Exception occurred while detaching from the remote JVM");
  }
}
