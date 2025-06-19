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
