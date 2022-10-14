#include <iostream>
#include <cassert>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <cctype>
#include <vector>
#include <map>
#include <algorithm>

// DEBUG FUNCTION
template<class T>
void print(T a) 
{
    for (auto ch : a) {
        std::cout << ch << " ";
    }
    std::cout << std::endl;
    return;
}

void print_map(std::map<char, std::vector<std::string>> a)
{
    for (auto item : a) {
        std::cout << item.first << "->";
        print(item.second);
    }
}

///////////////////////////////////////////



const char *grammarPath = "./grammar.txt";
const char *chomskyPath = "./Chomsky.txt";
const char *greibachPath = "./Greibach.txt";

std::set<char> V, T; // 分别为非终结符和终结符
std::map<char, std::vector<std::string>> P; // 产生式
char START = 'S'; // 开始符号


// 消除无用符号, 从S遍历非终结符，遍历到的存入checked，最终与V做差集，去除差集里的产生式
void remove_useless_symbols()
{
    std::set<char> checked, unchecked{START};
    while (!unchecked.empty()) {
        char check = 0;
        if (unchecked.count(START) == 1) {
            check = START;
        }
        else {
            auto it = unchecked.begin();
            check = *it;
        }
        for (const auto &s : P[check]) {
            for (auto ch : s) {
                if (std::isupper(ch) && checked.count(ch) == 0) {
                    unchecked.insert(ch);
                }
            }
        }
        checked.insert(check);
        auto it = unchecked.find(check);
        unchecked.erase(it);
    }
    std::vector<char> tmp;
    std::set_difference(V.begin(), V.end(), checked.begin(), checked.end(), std::back_inserter(tmp));
    for (auto ch : tmp) {
        auto it = P.find(ch);
        P.erase(it);
    }
    return;
}

// 消除单一产生式，递归消除替换，checked记录已消除的产生式左部
void remove_single_production(std::vector<std::string> &svec, std::set<char> &checked, char ch) {
    std::vector<std::string> tmp;
    for (auto s : svec) {
        if (s.size() == 1 && std::isupper(s[0])) {
            remove_single_production(P[s[0]], checked, s[0]);
            for (auto s1 : P[s[0]]) {
                tmp.push_back(s1);
            }
        }
        else {
            tmp.push_back(s);
        }
    }
    svec = tmp;
    checked.insert(ch);
    return;
}


void remove_empty_production()
{
    std::set<char> tmp;
    for (auto &item : P) {
        for (auto it = item.second.begin(); it != item.second.end(); ++it) {
            if (*it == "#") {
                tmp.insert(item.first);
                item.second.erase(it);
                break;
            }
        }
    }
    

}


/**
 *  @brief  translate the common grammar to chomsky normal forms.
 *  @param fin the file inlcuding the grammar
 *  @param fout the file to save chomsky normal forms
 */
void to_chomsky(std::fstream &fin, std::fstream &fout)
{
    // 从文件读取文法，解析存储
    std::string line;
    while (fin >> line) {

        for (int i = 0; i < line.size(); ++i) { // 构造非终结符集V和终结符集T
            if (i != 1 && i != 2 && line[i] != '|') {
                if (std::isupper(line[i]))
                    V.insert(line[i]);
                else
                    T.insert(line[i]);
            }
        }

        // 拆分，例如'S->aABC|a' 变为 'S aABC a' ，存储
        P.insert({line[0], std::vector<std::string>()});
        int preind = 3, ind = 3;
        std::cout << line << std::endl;
        while ((ind = line.find_first_of("|", ind, line.size() - ind)) != std::string::npos) {
            P[line[0]].push_back(std::string(line, preind, ind - preind));
            preind = ++ind;
        }
        P[line[0]].push_back(std::string(line, preind, line.size() - preind));
        for (auto val : P[line[0]]) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // 消除无用符号
    remove_useless_symbols();    
    
    // 消除单一产生式
    std::set<char> checked;
    for (auto &item : P) {
        if (checked.count(item.first) == 0)
            remove_single_production(item.second, checked, item.first);
    }

    // 消除#产生式
    remove_empty_production();

    return;
}

// void to_greibach(std::fstream &fin, std::fstream &fout)
// {

//     return;
// }


int main() {
    std::fstream grammar(grammarPath, std::ios_base::in);
    std::fstream chomsky(chomskyPath, std::ios_base::trunc | std::ios_base::in | std::ios_base::out);
    std::fstream greibach(greibachPath, std::ios_base::in);


    if (!grammar.is_open() || !chomsky.is_open() || !greibach.is_open())
        std::cout << "Error when it trys to open files!!!" << std::endl;
    else {
        to_chomsky(grammar, chomsky);
        chomsky.seekp(0); //move file pointer to the begin of the file
        // to_greibach(chomsky, greibach);

    }
    return 0;
}