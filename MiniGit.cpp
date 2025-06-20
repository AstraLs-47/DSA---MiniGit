#include "MiniGit.hpp"
#include "sha1.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

// Color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

MiniGit::MiniGit() { load(); }

void MiniGit::init() {
    fs::create_directory(".minigit");
    fs::create_directory(".minigit/objects");
    fs::create_directory(".minigit/commits");
    head = "master";
    branches.clear();
    branches["master"] = "";
    stagedFiles.clear();
    commits.clear();
    save();
    std::cout << GREEN << "Initialized empty MiniGit repository." << RESET << "\n";
}


void MiniGit::branch(const std::string& branchName) {
    branches[branchName] = branches[head]; // New branch points to current commit
save();
std::cout << GREEN << "Created branch: " << branchName << RESET << "\n";
}




void MiniGit::load() {
    // Load HEAD
    std::ifstream headin(".minigit/HEAD");
    if (headin) {
        std::getline(headin, head);
        headin.close();
    }

    // Load branches
    branches.clear();
    std::ifstream bin(".minigit/branches.txt");
    if (bin) {
        std::string name, hash;
        while (bin >> name >> hash)
            branches[name] = hash;
        bin.close();
    }

    // Load staged files
    stagedFiles.clear();
    std::ifstream sin(".minigit/staged.txt");
    if (sin) {
        std::string f;
        while (std::getline(sin, f))
            if (!f.empty()) stagedFiles.insert(f);
        sin.close();
    }

    // Load commits
    commits.clear();
    if (fs::exists(".minigit/commits")) {
        for (const auto&entry :fs::directory_iterator(".minigit/commits")) {
            std::ifstream cin(entry.path());
            if (cin) {
                Commit c;
                std::string n_blobs;
                std::getline(cin, c.hash);
                std::getline(cin, c.message);
                std::getline(cin, c.timestamp);
                std::getline(cin, c.parent);
                std::getline(cin, n_blobs);

                size_t n = 0;
                try {
                    n = std::stoul(n_blobs);
                } catch (const std::exception&) {
                    n = 0;
                }

                for (size_t i = 0; i < n; ++i) {
                    Blob b;
                    cin >>b.hash>>b.filename;
                    cin.ignore();
                    c.blobs.push_back(b);
                }
                commits[c.hash] = c;
            }
        }
    }
}
void MiniGit::commit(const std::string& message) {
    if (stagedFiles.empty()) {
        std::cout << YELLOW << "Nothing to commit." << RESET << "\n";
        return;
    }
    Commit c;
    c.message = message;
    c.timestamp = getTimestamp();
    c.parent = branches[head];
    for (const auto& file : stagedFiles) {
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            std::cerr << RED << "Error: Could not open file " << file << " for reading." << RESET << "\n";
            continue;
        }
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::string hash = sha1(content);
        saveBlob(file, hash);
        c.blobs.push_back({hash, file});
    }
    c.hash = sha1(c.message + c.timestamp + c.parent);
    commits[c.hash] = c;
    branches[head] = c.hash;
    stagedFiles.clear();
    save();
    std::cout << GREEN << "Committed: " << c.hash << RESET << "\n";
}

void MiniGit::diff(const std::string& commit1, const std::string& commit2) {
    if (!commits.count(commit1) || !commits.count(commit2)) {
        std::cout << RED << "One or both commits not found." << RESET << "\n";
        return;
    }
    const Commit& c1 = commits.at(commit1);
    const Commit& c2 = commits.at(commit2);

    std::map<std::string, std::string> files1, files2;
    for (const auto& b : c1.blobs) files1[b.filename] = b.hash;
    for (const auto& b : c2.blobs) files2[b.filename] = b.hash;

    std::set<std::string> allFiles;
    for (const auto& [f, _] : files1) allFiles.insert(f);
    for (const auto& [f, _] : files2) allFiles.insert(f);

    for (const auto& file : allFiles) {
        std::string h1 = files1.count(file) ? files1[file] : "";
        std::string h2 = files2.count(file) ? files2[file] : "";
        if (h1 == h2) continue;

        std::cout << YELLOW << "Diff for file: " << file << RESET << "\n";
        std::vector<std::string> lines1, lines2;
        if (!h1.empty()) {
            std::ifstream in1(".minigit/objects/" + h1);
            std::string line;
            while (std::getline(in1, line)) lines1.push_back(line);
        }
        if (!h2.empty()) {
            std::ifstream in2(".minigit/objects/" + h2);
            std::string line;
            while (std::getline(in2, line)) lines2.push_back(line);
        }
        size_t n = std::max(lines1.size(), lines2.size());
        for (size_t i = 0; i < n; ++i) {
            std::string l1 = i < lines1.size() ? lines1[i] : "";
            std::string l2 = i < lines2.size() ? lines2[i] : "";
            if (l1 != l2) {
                std::cout << RED << "- " << l1 << RESET << "\n";
                std::cout << GREEN << "+ " << l2 << RESET << "\n";
            }
        }
        std::cout << "-----------------------------\n";
    }
}


void MiniGit::show(const std::string& filename) const {
    std::ifstream in(filename);
    if (!in) {
        std::cout << RED << "File not found: " << filename << RESET << "\n";
        return;
    }
    std::cout << YELLOW << "----- " << filename << " -----" << RESET << "\n";
    std::string line;
    while (std::getline(in, line)) {
        std::cout << line << "\n";
    }
    std::cout << YELLOW << "-------------------------" << RESET << "\n";
}

void MiniGit::log() const {
    if (!branches.count(head)) {
        std::cout << RED << "No commits on this branch." << RESET << "\n";
        return;
    }
    std::string curr = branches.at(head);
    while (!curr.empty()) {
        if (!commits.count(curr)) break;
        const Commit& c = commits.at(curr);
        std::cout << CYAN << "Commit: " << c.hash << RESET << "\n";
        // Format date and time
        std::istringstream tsstream(c.timestamp);
        std::string weekday, month, day, time, year;
        tsstream >> weekday >> month >> day >> time >> year;
        std::cout << "Date:    " << month << " " << day << ", " << year << "\n";
        std::cout << "Time:    " << time << "\n";
        std::cout << "Message: " << BOLD << c.message << RESET << "\n";
        std::cout << "Files:\n";
        for (const auto& blob : c.blobs) {
            std::cout << "  " << blob.filename << "\n";
        }
        std::cout << "-----------------------------\n";
        curr = c.parent;
    }
}
void MiniGit::save() {
    // Save HEAD
    std::ofstream headout(".minigit/HEAD");
    headout << head << "\n";
    headout.close();

    // Save branches
    std::ofstream bout(".minigit/branches.txt");
    for (const auto& [name, hash] : branches)
        bout << name << " " << hash << "\n";
    bout.close();

    // Save staged files
    std::ofstream sout(".minigit/staged.txt");
    for (const auto& f : stagedFiles)
        sout << f << "\n";
    sout.close();

    // Save each commit as a separate file
    fs::create_directory(".minigit/commits");
    for (const auto& [hash, c] : commits) {
        std::ofstream cout(".minigit/commits/" + hash + ".txt");
        cout << c.hash << "\n";
        cout << c.message << "\n";
        cout << c.timestamp << "\n";
        cout << c.parent << "\n";
        cout << c.blobs.size() << "\n";
        for (const auto& b : c.blobs)
            cout << b.hash << " " << b.filename << "\n";
        cout.close();
    }
}

void MiniGit::merge(const std::string& branchName) {
    if (!branches.count(branchName)) {
        std::cout << RED << "Branch not found: " << branchName << RESET << "\n";
        return;
    }
    if (branchName == head) {
        std::cout << YELLOW << "You cannot merge a branch into itself." << RESET << "\n";
        return;
    }
    std::string currentCommitHash = branches[head];
    std::string targetCommitHash = branches[branchName];

    if (!commits.count(currentCommitHash) || !commits.count(targetCommitHash)) {
        std::cout << RED << "Invalid branches." << RESET << "\n";
        return;
    }

    std::cout << CYAN << "Merging branch '" << branchName << "' INTO current branch '" << head << "'." << RESET << "\n";

    const Commit& current = commits.at(currentCommitHash);
    const Commit& target = commits.at(targetCommitHash);

    std::map<std::string, std::string> currentFiles, targetFiles;
    for (const auto& b : current.blobs) currentFiles[b.filename] = b.hash;
    for (const auto& b : target.blobs) targetFiles[b.filename] = b.hash;

    std::set<std::string> allFiles;
    for (const auto& [f, _] : currentFiles) allFiles.insert(f);
    for (const auto& [f, _] : targetFiles) allFiles.insert(f);

    bool conflict = false;
    for (const auto& file : allFiles) {
        std::string curHash = currentFiles.count(file) ? currentFiles[file] : "";
        std::string tarHash = targetFiles.count(file) ? targetFiles[file] : "";

        if (curHash == tarHash) continue; // No change

        if (!curHash.empty() && !tarHash.empty() && curHash != tarHash) {
            // Conflict: both branches changed the file
            std::cout << RED << "CONFLICT: File '" << file << "' was modified in both branches." << RESET << "\n";
            conflict = true;
        } else if (curHash.empty() && !tarHash.empty()) {
            // File only in target: merge it
            stagedFiles.insert(file);
            std::ifstream in(".minigit/objects/" + tarHash, std::ios::binary);
            if (in) {
                std::ofstream out(file, std::ios::binary);
                out << in.rdbuf();
            }
            std::cout << GREEN << "Merged file '" << file << "' from branch '" << branchName << "' into current branch '" << head << "'." << RESET << "\n";
        }
        // If only in current, do nothing
    }

    if (conflict) {
        std::cout << YELLOW << "Merge completed with conflicts. Please resolve them and commit manually." << RESET << "\n";
    } else if (!stagedFiles.empty()) {
        std::cout << GREEN << "Merge successful. Please commit the result to complete the merge." << RESET << "\n";
    } else {
        std::cout << YELLOW << "Nothing to merge. The branches are already up to date." << RESET << "\n";
    }
}

void MiniGit::add(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cout << RED << "File not found: " << filename << RESET << "\n";
        return;
    }
    stagedFiles.insert(filename);
    std::cout << GREEN << "Staged: " << filename << RESET << "\n";
}
