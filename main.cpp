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
#include <cmath>
#include <iterator>

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

// 移除空产生式
void remove_empty_production()
{
    std::set<char> tmp;
    // 找到空产生式对应的非终结符
    for (auto &item : P) {
        if (item.first != START) {
            for (auto it = item.second.begin(); it != item.second.end(); ++it) {
                if (*it == "#") {
                    tmp.insert(item.first);
                    item.second.erase(it);
                    break;
                }
            }
        }
    }
    
    for (auto &item : P) {
        std::vector<std::string> res;
        for (const auto &s : item.second) {
            res.push_back(s);
            std::vector<int> ind;
            for (int i = 0; i < s.size(); ++i) {
                if (tmp.count(s[i]) == 1) {
                    ind.push_back(i);
                }
            }
            if (ind.empty())
                continue;
            
            // 根据目标非终结符个数n，构造n位，遍历2^n种可能，某位为0时，erase对应ind的符号
            for (int i = 0; i < (int)exp2(ind.size()); ++i) {
                std::string ss(s);
                for (int j = 0; j < ind.size(); ++j) {
                    int flag = (i >> j) & 1;
                    if (flag == 0)
                        ss.erase(ind[ind.size() - j - 1], 1);
                }
                if (std::find(item.second.begin(), item.second.end(), ss) == item.second.end()) {
                    res.push_back(ss);
                }
            }
        }
        item.second = res;
    }
    return;
}


// 构造G1
void construct_G1(char &candidate)
{
    std::map<char, char> rextraP; // reverse extra P
    for (auto &item : P) {
        std::vector<std::string> tmp;
        for (const auto &s : item.second) {
            if (s.size() == 1 && T.count(s[0]) == 1)
                tmp.push_back(s);
            else if (s.size() > 1) {
                auto ss(s);
                for (auto &ch : ss) {
                    if (T.count(ch) == 1) {
                        if (rextraP.count(ch) == 1)
                            ch = rextraP[ch];
                        else {
                            while (V.count(candidate) == 1) {
                                ++candidate;
                                assert(candidate <= 'Z');
                            }
                            rextraP.insert({ch, candidate});
                            V.insert(candidate);
                            ch = candidate;
                        }
                    }
                }
                tmp.push_back(ss);
            }
            item.second = tmp;
        }
    }
    for (const auto &item : rextraP) {
        P.insert({item.second, {std::string(1, item.first)}});
    }
    return;
}

// 构造GC
void construct_GC(char &candidate)
{
    std::map<std::string, char> rextraP;
    for (auto &item : P) {
        std::vector<std::string> tmp;
        for (const auto &s : item.second) {
            if (s.size() <= 2) {
                tmp.push_back(s);
            }
            else {
                bool firstFlag = true, breakFlag = false;
                char preChoseV = 0;
                auto ss(s);
                while (ss.size() > 2) {
                    char ch = ss[0];
                    char newV = 0;
                    ss.erase(0, 1);
                    if (rextraP.count(ss) == 1) {
                        newV = rextraP[ss];
                        breakFlag = true;
                    }
                    else {
                        while (V.count(candidate) == 1) {
                            ++candidate;
                            assert(candidate <= 'Z');
                        }
                        newV = candidate;
                    }
                    if (firstFlag) {
                        tmp.push_back(std::string(1, ch) + newV);
                        firstFlag = false;
                    }
                    else
                        rextraP.insert({std::string(1, ch) + newV, preChoseV});
                    if (breakFlag) {
                        break;
                    }
                    V.insert(newV);
                    preChoseV = newV;
                }
                if (!breakFlag)
                    rextraP.insert({ss, preChoseV});
            }
        }
        item.second = tmp;
    }
    for (const auto &item : rextraP) {
        P.insert({item.second, {item.first}});
    }
    return;
}



/**
 *  @brief  translate the common grammar to chomsky normal forms.
 *  @param fout the file to save chomsky normal forms
 */
void to_chomsky(std::fstream &fout)
{
    // 消除#产生式
    remove_empty_production();

    // 消除单一产生式
    std::set<char> checked;
    for (auto &item : P) {
        if (checked.count(item.first) == 0)
            remove_single_production(item.second, checked, item.first);
    }

    // 消除无用符号
    remove_useless_symbols();

    // 构造G1
    char candidate = 'A'; // 候选非终结符
    construct_G1(candidate);

    // 构造GC
    construct_GC(candidate);

    // 输出到文件
    for (const auto &item : P) {
        fout << item.first << "->";
        for (int i = 0; i < item.second.size(); ++i) {
            fout << item.second[i];
            if (i != item.second.size() - 1)
                fout << "|";
        }
        fout << std::endl;
    }
    return;
}



void search(std::vector<char> &res, char ch, bool &breakFlag)
{
    for (const auto &s : P[ch]) {
        if (V.count(s[0]) == 1) {
            auto it = std::find(res.begin(), res.end(), s[0]);
            if (it == res.end()) {
                res.push_back(s[0]);
                search(res, s[0], breakFlag);
                if (breakFlag)
                    return;
            }
            else {
                res.push_back(s[0]);
                breakFlag = true;
                return;
            }
        }
    }
    res.pop_back();
    return;
}





// 消除左递归产生式
void remove_left_recursion()
{
    char start = START;
    std::set<char> checked;
    while (true) {
        std::vector<char> haveRing{start};
        bool breakFlag = false;
        search(haveRing, start, breakFlag);  // 查找start开头的可达路线上是否有环

        if (breakFlag == false) { // 找不到的情况
            checked.insert(start);
            std::vector<char> tmp;
            std::set_difference(V.begin(), V.end(), checked.begin(), checked.end(), std::back_inserter(tmp));
            if (tmp.empty())
                break;
            start = *(tmp.begin()); // 找其他非终结符开头的情况
            continue;
        }

        // print(haveRing);
        // 找到的情况，把环上相关产生式放入tmpP
        decltype(P) tmpP;
        assert(haveRing.size() > 1);
        for (int i = haveRing.size() - 2; i >= 0; --i) {
            tmpP.insert({haveRing[i + 1], P[haveRing[i + 1]]});
            if (haveRing[i] == haveRing[haveRing.size() - 1])
                break;
        }
        // print_map(tmpP);
        // 代入
        auto tail = tmpP.end();
        std::advance(tail, -1);
        auto &firstItem = *tail;
        for (int i = 0; i < tmpP.size() - 1; ++i) {
            std::vector<std::string> tmp;
            for (const auto &s : firstItem.second) {
                if (s[0] != firstItem.first && tmpP.count(s[0]) == 1) {
                    for (const auto &ss : tmpP[s[0]]) {
                        auto tmps = s;
                        tmps.replace(tmps.begin(), tmps.begin() + 1, ss.begin(), ss.end());
                        tmp.push_back(tmps);
                    }
                }
                else
                    tmp.push_back(s);
            }
            firstItem.second = tmp;
        }

        // 消除直接左递归
        std::vector<std::string> a, b;
        for (const auto &s : firstItem.second) {
            if (s[0] == firstItem.first)
                a.push_back(std::string(s.begin() + 1, s.end()));
            else
                b.push_back(s);
        }
        std::vector<std::string> tmp(b);
        char newV = 'A';
        while (V.count(newV) == 1) {
            ++newV;
            assert(newV <= 'Z');
        }
        for (const auto &s : b) {
            tmp.push_back(s + newV);
        }
        firstItem.second = tmp;
        tmp = a;
        for (const auto &s : a) {
            tmp.push_back(s + newV);
        }
        V.insert(newV);
        tmpP[newV] = tmp;

        print_map(tmpP);

        // 回代其他产生式，消除间接左递归
        char ch = firstItem.first;
        for (int i = 0; i < tmpP.size() - 1; ++i) {
            for (auto &item : tmpP) {
                if (item.first != firstItem.first) {
                    tmp.clear();
                    bool flag = false;
                    for (const auto &s : item.second) {
                        if (s[0] == ch) {
                            for (const auto &ss : tmpP[ch]) {
                                tmp.push_back(ss + std::string(s.begin() + 1, s.end()));
                            }
                            flag = true;
                        }
                        else {
                            tmp.push_back(s);
                        }
                    }
                    item.second = tmp;
                    if (flag) {
                        ch = item.first;
                    }
                }
            }
        }
        for (const auto &item : tmpP) {
            P[item.first] = item.second;
        }
    }
    print_map(P);
    return;
}

void to_greibach(std::fstream &fout)
{
    // 消除#产生式
    remove_empty_production();

    // 消除单一产生式
    std::set<char> checked;
    for (auto &item : P) {
        if (checked.count(item.first) == 0)
            remove_single_production(item.second, checked, item.first);
    }

    // 消除无用符号
    remove_useless_symbols();

    print_map(P);
    std::cout << std::endl;
    
    // 消除左递归
    remove_left_recursion();

    // print_map(P);

    return;
}


int main() {
    std::fstream grammar(grammarPath, std::ios_base::in);
    std::fstream chomsky(chomskyPath, std::ios_base::trunc | std::ios_base::in | std::ios_base::out);
    std::fstream greibach(greibachPath, std::ios_base::in);


    if (!grammar.is_open() || !chomsky.is_open() || !greibach.is_open())
        std::cout << "Error when it trys to open files!!!" << std::endl;
    else {
        // 从文件读取文法，解析存储
        std::string line;
        while (grammar >> line) {

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
            // std::cout << line << std::endl;
            while ((ind = line.find_first_of("|", ind)) != std::string::npos) {
                P[line[0]].push_back(std::string(line, preind, ind - preind));
                preind = ++ind;
            }
            P[line[0]].push_back(std::string(line, preind, line.size() - preind));
            std::cout << std::string(line, preind, line.size() - preind) << std::endl;
            // for (auto val : P[line[0]]) {
            //     std::cout << val << " ";
            // }
            // std::cout << std::endl << std::endl;
        }

        // auto P_backup = P;
        // auto V_backup = V;
        // to_chomsky(chomsky);

        // P = P_backup;
        // V = V_backup;
        to_greibach(greibach);

    }
    return 0;
}