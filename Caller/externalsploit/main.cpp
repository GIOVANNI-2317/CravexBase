#include "Init.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// Command dispatcher for future-proofing
int main(int argc, char* argv[]) {
    // Basic structure: externalsploit.exe [Command] [Action] [Data]
    // Example: externalsploit.exe "print" "info" "Hello World!"

    if (argc < 2) {
        return 0;
    }

    std::string command = argv[1];

    if (command == "print") {
        if (argc < 3) return 0; // Need an action (like "info")
        
        std::string action = argv[2];
        if (action == "info") {
            if (argc < 4) return 0; // Need the message data

            // JOINING LOGIC: Join without spaces as requested (test something -> testsomething)
            std::string full_msg;
            for (int i = 3; i < argc; ++i) {
                full_msg += argv[i];
                // No space added between arguments
            }

            // Copy to the global buffer for the shellcode
            strncpy_s(g_print_msg, sizeof(g_print_msg), full_msg.c_str(), _TRUNCATE);
            g_print_typ = 1;

            // Execute the dynamic print shellcode
            execute_function(dynamic_print);
        }
    }
    else {
        execute_function(test_print);
    }

    return 0;
}