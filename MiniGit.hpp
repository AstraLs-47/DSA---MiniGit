#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
struct Blob {    
  std::string hash;
  std::string filename;
};
struct Commit {
  std::string hash;
  std::string message;
  std::string timestamp;
  std::string parent;
  std::vector<Blob> blobs;
};
class MiniGit {
public:
  MiniGit();
  void init();
  void add(const std::string& filename);
  void commit(const std::string& message);
  void log() const;
  void branch(const std::string& branchName);
  void checkout(const std::string& ref);
  void status() const;
  void merge(const std::string& branchName);
  void diff(const std::string& commit1, const std::string& commit2);
  void show(const std::string& filename) const;
private:
  std::string head;
  std::map<std::string, Commit> commits; 
  std::map<std::string, std::string> branches;
  std::set<std::string> stagedFiles; 
  void saveBlob(const std::string& filename, const std::string& hash);
  std::string getTimestamp() const;
  void save(); 
  void load();
};
