#include <string_view>

#include <jni.h>
#include <unistd.h>

/**
 * Represents the attachment between the calling process and a remote
 * JVM instance.
 */
class JvmAttachment {
private:
  /**
   * Pointer to the JVM instance initialized on object creation.
   */
  JavaVM* jvm;

  /**
   * Pointer to the JNI interface function table for calling into the JVM.
   */
  JNIEnv* jni_env;

  /**
   * A JNI reference to a com/sun/tools/attach/VirtualMachine object for remote
   * JVM management.
   */
  jobject attachment;

  /**
   * Checks the global JNI state to see if there is a current exception condition,
   * and, if so, throws an exception.
   *
   * @throws std::runtime_error if JNI detects an exception in the JVM
   */
  void abort_if_jvm_exception() const;

  /**
   * Attempts to load the given class from the current JVM instance.
   *
   * @return reference to the loaded class
   * @throws std::runtime_error if the class cannot be loaded/found
   */
  jclass load_class(std::string_view) const;

  /**
   * Attempts to load the given instance method for a class from the
   * current JVM instance.
   *
   * @param jclass class reference to load the method for
   * @param std::string_view name of the method
   * @param std::string_view signature of the method, in JLS notation
   * @return reference to the found method
   * @throws std::runtime_error if the method could not be found
   */
  jmethodID load_method(jclass, std::string_view, std::string_view) const;

  /**
   * Attempts to load the given static method for a class from the current
   * JVM instance.
   *
   * @param jclass class reference to load the method for
   * @param std::string_view name of the method
   * @param std::string_view signature of the method, in JLS notation
   * @return reference to the found method
   * @throws std::runtime_error if the method could not be found
   */
  jmethodID load_static_method(jclass, std::string_view, std::string_view) const;
public:
  // Default constructor/deconstructor
  JvmAttachment();
  ~JvmAttachment();

  // No copy/move constructors
  JvmAttachment(const JvmAttachment&) = delete;
  JvmAttachment(JvmAttachment&&) = delete;

  // No copy/move operators
  JvmAttachment& operator=(const JvmAttachment&) = delete;
  JvmAttachment& operator=(JvmAttachment&&) = delete;

  /**
   * Attempts to attach to the given PID on the local system for further
   * operations.
   *
   * @param pid_t the local sytem PID to attempt attaching to
   * @throws std::runtime_error if the attachment failed or alreadt attached
   */
  void attach(pid_t);

  /**
   * Detaches from the connected remote JVM, allowing other attachments
   * to occur.
   *
   * When an instance of this class is not attached, this function is
   * a no-op.
   *
   * @throws std::runtime_error when detaching results in a JVM exception
   */
  void detach();

  /**
   * Given a provided agent library name, attempts to load the shared library
   * into the remote JVM that is currently attached.
   *
   * The remote process will need to have the library on it's linking path
   * which can be done through environment variables such as LD_LIBRARY_PATH
   * on Linux or DYLD_LIBRARY_PATH on MacOS. Consult your dynamic linker's
   * documentation to determine the proper configuration.
   *
   * @throws std::runtime_error if loading results in a JVM exception
   */
  void load_agent(std::string_view) const;  
};
