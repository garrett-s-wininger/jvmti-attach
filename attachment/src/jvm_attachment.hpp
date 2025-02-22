#include <jni.h>
#include <unistd.h>

/**
 * Represents the attachment between the calling process and a remote
 * JVM instance.
 */
class JvmAttachment {
private:
  JavaVM* jvm;
  JNIEnv* jni_env;

  jclass virtual_machine;
  jclass virtual_machine_impl;

  jmethodID attach;
  jmethodID detach;
public:
  JvmAttachment();
  ~JvmAttachment();

  // No copy/move constructors
  JvmAttachment(const JvmAttachment&) = delete;
  JvmAttachment(JvmAttachment&&) = delete;

  // No copy/move operators
  JvmAttachment& operator=(const JvmAttachment&) = delete;
  JvmAttachment& operator=(JvmAttachment&&) = delete;

  /**
   * Attaches the process to the remote JVM instance specified
   * by the `java_proc` parameter.
   *
   * This function currently attaches and then immediately
   * detaches as a demonstrator of the processes required
   * up to the point of loading a given agent.
   *
   * @param java_proc PID of the remote JVM process to attach to
   */
  void attach_to_remote(pid_t java_proc);  
};
