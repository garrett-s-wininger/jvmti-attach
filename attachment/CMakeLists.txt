project(attachment)

enable_language(CXX)

add_executable(
  jvmti-attach
  src/main.cpp
  src/jvm_attachment.cpp
)

target_compile_features(jvmti-attach PRIVATE cxx_std_23)
target_link_libraries(jvmti-attach PRIVATE jvm)
