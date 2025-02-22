#include <vector>

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Obtains a listing of Java process identifiers currently running on the
 * system.
 *
 * This implementation uses the MacOS-specific `sysctl` implementation in
 * order to obtain the listing which will need to be reimplemented for use
 * on other operating systems.
 *
 * @returns listing of Java process identifiers on the local system
 */
std::vector<pid_t> snapshot_java_procs() {
  // MIB query, procs for current user
  std::array<int, 4> mib = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_UID,
    static_cast<int>(getuid())
  };

  std::vector<struct kinfo_proc> proc_list;
  size_t required_allocation;

  // Dynamic allocation reservation
  sysctl(mib.data(), mib.size(), nullptr, &required_allocation, nullptr, 0);
  size_t proc_entries = required_allocation / sizeof(proc_list);

  struct kinfo_proc placeholder = {};
  memset(&placeholder, 0, sizeof(placeholder));
  proc_list.resize(proc_entries, placeholder);

  // Populate kernel process info snapshot
  int status = sysctl(
    mib.data(),
    mib.size(),
    proc_list.data(),
    &required_allocation,
    nullptr,
    0
  );

  if (status == -1) {
    throw std::runtime_error("Failed to obtain OS process tree snapshot");
  }

  std::erase_if(
    proc_list,
    [](struct kinfo_proc proc) {
      return std::string_view(proc.kp_proc.p_comm).compare("java") != 0;
    }
  );

  // Transform into the raw PIDs from the MacOS kernel data structure
  std::vector<pid_t> pids;

  std::transform(
    proc_list.begin(),
    proc_list.end(),
    std::back_inserter(pids),
    [](auto proc) { return proc.kp_proc.p_pid; }
  );
  
  return pids;
}

