#include "MiniGit.hpp"
#include <iostream>
#include <sstream>
using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

int main() {
    // Banner and welcome message
    std::cout << YELLOW << "=============================\n";
    std::cout << "      Welcome to MiniGit     \n";
    std::cout << "=============================" << RESET << "\n";
    std::cout << CYAN << "Type 'commands' for commands." << RESET << "\n";

    MiniGit mg;
    std::string line;
    while (true) {
        std::cout << BOLD << CYAN << "> " << RESET;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        std::string cmd, arg1, arg2;
        iss >> cmd >> arg1;
        std::getline(iss, arg2);

        if (cmd == "init") mg.init();
        else if (cmd == "add") mg.add(arg1);
        else if (cmd == "commit") {
            size_t pos = line.find("-m");
            std::string msg = "No message";
            if (pos != std::string::npos) {
                size_t start = line.find("\"", pos);
                size_t end = line.rfind("\"");
                if (start != std::string::npos && end != std::string::npos && end > start)
                    msg = line.substr(start + 1, end - start - 1);
            }
            mg.commit(msg);
        }
        else if (cmd == "log") mg.log();
        else if (cmd == "branch") mg.branch(arg1);
        else if (cmd == "checkout") mg.checkout(arg1);
        else if (cmd == "status") mg.status();
        else if (cmd == "merge") mg.merge(arg1);
        else if (cmd == "diff") {
            std::string c1, c2;
            iss.clear();
            iss.str(arg1 + arg2);
            iss >> c1 >> c2;
            mg.diff(c1, c2);
        }
        else if (cmd == "show") mg.show(arg1);
        else if (cmd == "exit"){ 
            cout << RED << "Exiting MiniGit....." << RESET << "\n";
            break;}
        else if (cmd == "commands") {
            std::cout << YELLOW << "Commands:\n"
                << CYAN << "  init" << RESET << "                - Initialize repository\n"
                << CYAN << "  add <file>" << RESET << "          - Stage file\n"
                << CYAN << "  commit -m \"msg\"" << RESET << "   - Commit staged files\n"
                << CYAN << "  log" << RESET << "                 - Show commit log\n"
                << CYAN << "  branch <name>" << RESET << "       - Create branch\n"
                << CYAN << "  checkout <ref>" << RESET << "      - Switch branch/commit\n"
                << CYAN << "  status" << RESET << "              - Show status\n"
                << CYAN << "  merge <branch>" << RESET << "      - Merge branch \n"
                << CYAN << "  diff <c1> <c2>" << RESET << "      - Diff two commits \n"
                << CYAN << "  show <file>" << RESET << "         - Display file content\n"
                << CYAN << "  exit" << RESET << "                - Exit\n";
        }
        else std::cout << RED << "Unknown command." << RESET << "\n";
    }
    return 0;
}
