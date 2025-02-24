#include <iostream>

#ifdef __MACH__
#include "arch/darwin.hpp"
#endif

#include "jvm_attachment.hpp"

/**
 * Requests a selection from the user in order to determine which running
 * Java process should be monitored.
 *
 * @param proc_list a listing of active Java processes
 * @returns PID of the process the user wishes to monitor
 */
pid_t select_proc_to_monitor(std::vector<pid_t>& proc_list) {
  // Header output
  std::println("Available Java Processes");
  std::println("------------------------\n");

  for (auto proc : proc_list) {
    std::println("{0}", proc);
  }

  std::println("");

  // Input loop
  pid_t proc_to_attach;

  do {
    std::print("Which one would you like to monitor? ");
    std::cin >> proc_to_attach;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  } while (
    proc_to_attach == 0
    || std::find_if(
      proc_list.begin(),
      proc_list.end(),
      [proc_to_attach](auto proc) { return proc == proc_to_attach; }
    ) == proc_list.end()
  );

  return proc_to_attach;
}

int main(int argc, char** argv) {
  try {
    // Get Java process listing
    auto proc_list = snapshot_java_procs();

    // Abort if no processes found
    if (proc_list.empty()) {
      std::println("No Java processes found.");
      exit(EXIT_SUCCESS);
    }

    // Get choice for monitoring selection
    pid_t proc_to_monitor;

    if (proc_list.size() == 1) {
      proc_to_monitor = proc_list.at(0);
    } else {
      proc_to_monitor = select_proc_to_monitor(proc_list);
    }

    std::println("Selected PID: {0}", proc_to_monitor);

    // Attach to the remote JVM
    JvmAttachment driver = JvmAttachment();
    driver.attach(proc_to_monitor);
    driver.load_agent("<TODO>"); // TODO(Garrett): Parse user input for lib
    driver.detach();
  } catch (const std::runtime_error& e) {
    std::println("\nError:\n");
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }

  return 0;
}
