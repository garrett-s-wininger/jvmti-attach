#include <iostream>
#include <optional>

#include <unistd.h>

#ifdef __MACH__
#include "arch/darwin.hpp"
#else
/**
 * A fallback implementation returning an empty list of processes for systems
 * who's checks cannot be included at compile time.
 *
 * @return empty vector of process identifiers
 */
std::vector<pid_t> snapshot_java_procs() {
  std::println("[WARN] Process enumeration is not supported on this system");
  return std::vector<pid_t>();
}
#endif

#include "jvm_attachment.hpp"

/**
 * Arguments from the user to control the program.
 */
struct ProgramArgs {
  /**
   * The library the user wants to inject into the remote process.
   */
  std::string library;

  /**
   * PID to attach to.
  */
  pid_t monitor_pid;  
};

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

/**
 * Prints the usage of the program and then gracefully exits the application.
 */
void print_usage() {
  std::println(
    stderr,
    "JVM TI - A tool for dynamically attaching JVM agents at run time"
  );

  std::println(stderr, "\nUsage:\n");
  std::println(stderr, "{:<2}jvmti-attach {:<21} - {}", " ", "", "Interactive PID selection");

  std::println(
    stderr,
    "{:<2}jvmti-attach {:<21} - {}",
    " ",
    "-h",
    "Print this message and exit"
  );

  std::println(
    stderr,
    "{:<2}jvmti-attach {:<21} - {}",
    "",
    "-l <library> -p <pid>",
    "Non-interactive attachment"
  );

  std::println(stderr, "");
}

/**
 * Parses arguments provided by the user to control program flow.
 *
 * @param argc standard C/C++ main argument count
 * @param argv standard C/C++ main argument vector
 * @return optional argument struct derived from the user preferences
 */
std::optional<ProgramArgs> parse_args(int argc, char** argv) {
  int opt;
  size_t opt_len;
  ProgramArgs args = {std::string{}, 0};

  while ((opt = getopt(argc, argv, "hl:p:")) != -1) {
    switch (opt) {
      case 'l':
        args.library = std::string(optarg);
        break;
      case 'p':
        opt_len = strlen(optarg);

        // Basic PID validation
        for (int i = 0; i < opt_len; ++i) {
          if (!isdigit(optarg[i])) {
            std::println(stderr, "Error:\n");
            std::println(stderr, "{} is not a valid PID, must be numeric", optarg);
            exit(EXIT_FAILURE);
          }
        }

        args.monitor_pid = atoi(optarg);

        if (args.monitor_pid == 0) {
          std::println(stderr, "Error:\n");
          std::println(stderr, "PID must be 1 or greater");
        }

        break;
      case 'h':
        print_usage();
        exit(EXIT_SUCCESS);
      default:
        print_usage();
        exit(EXIT_FAILURE);
    }
  }

  if (args.library.empty() && args.monitor_pid == 0) {
    // No provided choices, don't set the optional's value so we perform interactively
    return {};
  } else if (
      (!args.library.empty() && args.monitor_pid == 0)
      || (args.library.empty() && args.monitor_pid != 0)) {
    // We've provided one but not the other, don't allow to simplify later logic
    std::println(stderr, "Error:\n");
    std::println(stderr, "When one of library or PID are provided, the other is required.");
    exit(EXIT_FAILURE);
  }

  return args;
}

int main(int argc, char** argv) {
  // Basic control flow variable storage
  std::optional<ProgramArgs> args = parse_args(argc, argv);
  std::string library_to_attach;
  pid_t proc_to_monitor;

  try {
    // Perform our prompts since we don't have fallbacks
    if (!args.has_value()) {
      // Get Java process listing
      auto proc_list = snapshot_java_procs();

      // Abort if no processes found
      if (proc_list.empty()) {
        std::println("No Java processes found.");
        exit(EXIT_SUCCESS);
      }

      // Get choice for monitoring selection
      if (proc_list.size() == 1) {
        proc_to_monitor = proc_list.at(0);
      } else {
        proc_to_monitor = select_proc_to_monitor(proc_list);
      }

      std::print("What library will be attached? ");
      std::cin >> library_to_attach;
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::println("");
    } else {
      // Adhere to user wishes
      library_to_attach = args.value().library;
      proc_to_monitor = args.value().monitor_pid;
    }

    std::println("Selected PID: {}", proc_to_monitor);
    std::println("Selected Library: {}", library_to_attach);

    // Attach to the remote JVM
    JvmAttachment driver = JvmAttachment();
    driver.attach(proc_to_monitor);
    driver.load_agent(library_to_attach);
    driver.detach();
  } catch (const std::runtime_error& e) {
    std::println("\nError:\n");
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }

  return 0;
}
