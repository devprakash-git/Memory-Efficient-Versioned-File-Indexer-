#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <sstream>
using namespace std;


void toLowerCase(string& str) {
    for (char& c : str) {
        if (c >= 'A' && c <= 'Z') c += ('a' - 'A');
    }
}


class VersionedIndex {
private:
    unordered_map<string, int> wordCounts;
    string versionName;

public:
    VersionedIndex(const string& vName) : versionName(vName) {}

    void addWord(const string& word) {
        wordCounts[word]++;
    }

    void addWord(const string& word, int count) {
        wordCounts[word] += count;
    }

    int getFrequency(const string& word) const {
        auto it = wordCounts.find(word);
        if (it != wordCounts.end()) {
            return it->second;
        }
        return 0;
    }

    const unordered_map<string, int>& getCounts() const {
        return wordCounts;
    }
    
    string getVersionName() const { return versionName; }
};


class Tokenizer {
public:

    void tokenizeChunk(const char* buffer, size_t size, string& leftover, VersionedIndex& index) {
        for (size_t i = 0; i < size; ++i) {
            char c = buffer[i];
            if (isalnum(static_cast<unsigned char>(c))) {
                leftover += c;
            } else {
                if (!leftover.empty()) {
                    toLowerCase(leftover);
                    index.addWord(leftover);
                    leftover.clear();
                }
            }
        }
    }
    
    void finish(string& leftover, VersionedIndex& index) {
        if (!leftover.empty()) {
            toLowerCase(leftover);
            index.addWord(leftover);
            leftover.clear();
        }
    }
};


class BufferedFileReader {
private:
    size_t bufferSizeKB;
    size_t bufferSizeBytes;
    
public:
    BufferedFileReader(size_t sizeKB) : bufferSizeKB(sizeKB) {
        if (sizeKB < 256 || sizeKB > 1024) {
            throw runtime_error("Buffer size must be between 256 and 1024 KB.");
        }
        bufferSizeBytes = sizeKB * 1024;
    }

    void readFileAndIndex(const string& filepath, VersionedIndex& index) {
        ifstream file(filepath, ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + filepath);
        }

        vector<char> buffer(bufferSizeBytes);
        string leftover;
        Tokenizer tokenizer;

        while (file.read(buffer.data(), bufferSizeBytes) || file.gcount() > 0) {
            tokenizer.tokenizeChunk(buffer.data(), file.gcount(), leftover, index);
        }
        tokenizer.finish(leftover, index);
    }
    
    size_t getBufferSizeKB() const { return bufferSizeKB; }
};


template <typename K, typename V>
vector<pair<K, V>> sortMapByValue(const unordered_map<K, V>& map) {
    vector<pair<K, V>> vec(map.begin(), map.end());
    sort(vec.begin(), vec.end(), [](const pair<K, V>& a, const pair<K, V>& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first; 
    });
    return vec;
}


class QueryProcessor {
protected:
    size_t bufferSizeKB;
public:
    QueryProcessor(size_t bufKB) : bufferSizeKB(bufKB) {}
    virtual ~QueryProcessor() {}
    virtual void execute() = 0; 
    
    void printCommonFooter(double executionTime, size_t bufKB) {
        cout << "Buffer Size (KB): " << bufKB << endl;
        cout << "Execution Time (s): " << executionTime << endl;
    }
};

class WordQuery : public QueryProcessor {
private:
    string filepath;
    string version;
    string word;
    
public:
    WordQuery(size_t bufKB, const string& fp, const string& v, const string& w)
        : QueryProcessor(bufKB), filepath(fp), version(v), word(w) {
        toLowerCase(this->word);    
    }

    void execute() override {
        auto start = chrono::high_resolution_clock::now();
        
        VersionedIndex index(version);
        BufferedFileReader reader(bufferSizeKB);
        reader.readFileAndIndex(filepath, index);
        
        int count = index.getFrequency(word);
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diff = end - start;

        cout << "Version: " << version << endl;
        cout << "Count: " << count << endl;
        printCommonFooter(diff.count(), bufferSizeKB);
    }
};

class TopKQuery : public QueryProcessor {
private:
    string filepath;
    string version;
    int k;
    
public:
    TopKQuery(size_t bufKB, const string& fp, const string& v, int topK)
        : QueryProcessor(bufKB), filepath(fp), version(v), k(topK) {}

    void execute() override {
        auto start = chrono::high_resolution_clock::now();
        
        VersionedIndex index(version);
        BufferedFileReader reader(bufferSizeKB);
        reader.readFileAndIndex(filepath, index);
        
        auto sortedWords = sortMapByValue(index.getCounts());
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diff = end - start;
        

        cout << "Top " << k << " Words in Version:"<< version << endl;
        int count = 0;
        for (const auto& pair : sortedWords) {
            cout << pair.first << " - " << pair.second << endl;
            if (++count >= k) break;
        }
        
        printCommonFooter(diff.count(), bufferSizeKB);
    }
};

class DiffQuery : public QueryProcessor {
private:
    string file1, file2;
    string ver1, ver2;
    string word;
    
public:
    DiffQuery(size_t bufKB, const string& f1, const string& v1, const string& f2, const string& v2, const string& w)
        : QueryProcessor(bufKB), file1(f1), ver1(v1), file2(f2), ver2(v2), word(w) {
         toLowerCase(this->word);
    }

    void execute() override {
        auto start = chrono::high_resolution_clock::now();
        
        VersionedIndex index1(ver1);
        BufferedFileReader reader1(bufferSizeKB);
        reader1.readFileAndIndex(file1, index1);
        
        VersionedIndex index2(ver2);
        BufferedFileReader reader2(bufferSizeKB);
        reader2.readFileAndIndex(file2, index2);
        
        int count1 = index1.getFrequency(word);
        int count2 = index2.getFrequency(word);
        int diffCount = count2 - count1;  
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diffTime = end - start;
        
        cout << "Versions: " << ver1 << " and " << ver2 << endl;
        cout << "Difference for '" << word << "' (" << ver2 << " - " << ver1 << "): " << diffCount << endl;
        printCommonFooter(diffTime.count(), bufferSizeKB);
    }
};

int main(int argc, char* argv[]) {
    try {
        string file, file1, file2;
        string version, version1, version2;
        int bufferKB = 0;
        string queryType;
        string word;
        int topK = 0;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--file" && i + 1 < argc) file = argv[++i];
            else if (arg == "--file1" && i + 1 < argc) file1 = argv[++i];
            else if (arg == "--file2" && i + 1 < argc) file2 = argv[++i];
            else if (arg == "--version" && i + 1 < argc) version = argv[++i];
            else if (arg == "--version1" && i + 1 < argc) version1 = argv[++i];
            else if (arg == "--version2" && i + 1 < argc) version2 = argv[++i];
            else if (arg == "--buffer" && i + 1 < argc) bufferKB = stoi(argv[++i]);
            else if (arg == "--query" && i + 1 < argc) queryType = argv[++i];
            else if (arg == "--word" && i + 1 < argc) word = argv[++i];
            else if (arg == "--top" && i + 1 < argc) topK = stoi(argv[++i]);
        }

        if (bufferKB < 256 || bufferKB > 1024) {
            throw invalid_argument("Buffer size must be between 256 and 1024 KB.");
        }

        QueryProcessor* processor = nullptr;

        if (queryType == "word") {
            if (file.empty() || version.empty() || word.empty()) {
                throw invalid_argument("Missing arguments for word query. Required: --file, --version, --word");
            }
            processor = new WordQuery(bufferKB, file, version, word);
        } else if (queryType == "top") {
            if (file.empty() || version.empty() || topK <= 0) {
                throw invalid_argument("Missing/invalid arguments for top query. Required: --file, --version, --top (>0)");
            }
            processor = new TopKQuery(bufferKB, file, version, topK);
        } else if (queryType == "diff") {
            if (file1.empty() || version1.empty() || file2.empty() || version2.empty() || word.empty()) {
                throw invalid_argument("Missing arguments for diff query. Required: --file1, --version1, --file2, --version2, --word");
            }
            processor = new DiffQuery(bufferKB, file1, version1, file2, version2, word);
        } else {
            throw invalid_argument("Unknown or missing query type. Must be 'word', 'top', or 'diff'.");
        }

        if (processor) {
            processor->execute(); 
            delete processor;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
