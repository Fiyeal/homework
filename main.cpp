#include <ios>
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
#include <stack>

const char *grammarPath = "./grammar.txt";
const char *chomskyPath = "./Chomsky.txt";
const char *greibachPath = "./Greibach.txt";
const char *npdaPath = "./NPDA.txt";
const char *testPath = "./test.txt";

// 上下文无关文法
std::set<char> V, T; // 分别为非终结符和终结符
std::map<char, std::vector<std::string>> P; // 产生式
char START = 'S'; // 开始符号

// NPDA
struct state {
    std::string q;
    char ch;
    char top;

    bool operator<(const state &b) const
    {
        if (this->top != b.top)
            return this->top < b.top;
        else if (this->ch != b.ch)
            return this->ch < b.ch;
        else
            return this->q < b.q;
    }
};
struct behavior {
    std::string q;
    std::string in;
};
std::map<state, std::vector<behavior>> F{ //转移规则
    {{"q0", '#', 'z'}, {{"q1", "Sz"}}}, 
    {{"q1", '#', 'z'}, {{"q2", "z"}}}
};


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

void print_map(const std::map<state, std::vector<behavior>> &F)
{
    for (const auto &item : F) {
        std::cout << "f(" << item.first.q << ", " << item.first.ch << ", " << item.first.top << ") = {";
        for (int i = 0; i < item.second.size(); ++i) {
            auto b = item.second[i];
            std::cout << "(" << b.q << ", " << b.in << ")";
            if (i != item.second.size() - 1)
                std::cout << ", ";
        }
        std::cout << "}" << std::endl;
    }
}

///////////////////////////////////////////


// 消除无用符号, 从S遍历非终结符，遍历到的存入checked，最终与V做差集，去除差集里的产生式
void remove_useless_symbols()
{
    // 消除从S不可达的无用符号
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
        auto itV = V.find(ch);
        V.erase(itV);
    }

    // 消除不产生终结符的无用符号
    decltype(P) uselessP;
    for (const auto &item : P) {
        if (item.first == START)
            continue;
        bool flag = false;
        for (const auto s : item.second) {
            for (auto ch : s) {
                if (T.count(ch) == 1)
                    flag = true;
            }
            if (flag)
                break;
        }
        if (flag)
            break;
        uselessP.insert(item);
    }
    for (const auto &item : uselessP) {
        auto it = P.find(item.first);
        P.erase(it);
    }
    decltype(P) preP;
    while (preP != P) { // 直到P不再发生变化终止循环
        preP = P;
        for (auto &item : P) {
            std::vector<std::string> tmp;
            for (const auto s : item.second) {
                auto flag = false;
                for (auto it = s.begin(); it != s.end(); ++it) {
                    if (uselessP.count(*it) == 1) {
                        for (const auto &ss : uselessP[*it]) {
                            auto tmps = s;
                            tmp.push_back(tmps.replace(it, it + 1, ss));
                        }
                        flag = true;
                        break; // 一个字符一个字符的替换
                    }
                }
                if (flag) // 替换了，所以原式不push
                    continue;
                tmp.push_back(s);
            }
            item.second = tmp;
        }
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



// 输出到文件
void to_file(std::fstream &fout)
{
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
    to_file(fout);

    return;
}


// 搜索以ch开始的产生式是否存在环
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
void remove_left_recursion(char newV)
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

        // print_map(tmpP);

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
    // print_map(P);
    return;
}


// 消除右部开头V
void make_T_begin(char ch, std::vector<std::string> &svec, std::set<char> &checked)
{
    std::vector<std::string> tmp;
    for (const auto &s : svec) {
        if (V.count(s[0]) == 1) {
            if (checked.count(s[0]) == 0) {
                make_T_begin(s[0], P[s[0]], checked);
            }
            for (const auto &ss : P[s[0]]) {
                tmp.push_back(ss + std::string(s.begin() + 1, s.end()));
            }
        }
        else
            tmp.push_back(s);
    }
    svec = tmp;
    checked.insert(ch);
    return;
}

// 构造目标范式
void construct_Greibach(char newV)
{
    std::map<char, char> rextraP;
    for (auto &item : P) {
        for (auto &s : item.second) {
            for (int i = 0; i < s.size(); ++i) {
                if (i != 0 && V.count(s[i]) == 0) {
                    if (rextraP.count(s[i]) == 0) {
                        while (V.count(newV) == 1) {
                            ++newV;
                            assert(newV <= 'Z');
                        }
                        rextraP.insert({s[i], newV});
                        V.insert(newV);
                        s[i] = newV;
                    }
                    else {
                        s[i] = rextraP[s[i]];
                    }
                }
            }
        }
    }
    // 更新P
    for (const auto &item : rextraP) {
        P.insert({item.second, {std::string(1, item.first)}});
    }
    return;
}

// 上下文无关文法转换成Greibach范式
void to_greibach(std::fstream &fout)
{
    char newV = 'A'; // 用以产生没用过的非终结符

    decltype(P) preP;
    while (preP != P) { // 直到P不再变化
        preP = P;

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

        // 消除左递归
        remove_left_recursion(newV);
    }
    

    // 消除产生式右部开头为非终结符的情况
    std::set<char> checked;
    for (const auto &item : P) {
        bool flag = true;
        for (const auto &s : item.second) {
            if (V.count(s[0]) == 1) {
                flag = false;
                break;
            }
        }
        if (flag)
            checked.insert(item.first);
    }
    for (auto &item : P) {
        if (checked.count(item.first) == 0) {
            make_T_begin(item.first, item.second, checked);
        }
    }

    // 构造目标范式
    construct_Greibach(newV);

    // 写入文件
    to_file(fout);

    return;
}


void to_file(std::fstream &fout, const std::map<state, std::vector<behavior>> &F)
{
    for (const auto &item : F) {
        fout << "f(" << item.first.q << ", " << item.first.ch << ", " << item.first.top << ") = {";
        for (int i = 0; i < item.second.size(); ++i) {
            auto b = item.second[i];
            fout << "(" << b.q << ", " << b.in << ")";
            if (i != item.second.size() - 1)
                fout << ", ";
        }
        fout << "}" << std::endl;
    }
}

// 从Greibach范式构造NPDA
void to_NPDA(std::fstream &fout) {
    for (const auto &item : P) {
        for (const auto &s : item.second) {
            auto tmp = (s.size() == 1 ? std::string("#") : std::string(s.begin() + 1, s.end()));
            F[{"q1", s[0], item.first}].push_back({"q1", tmp});
        }
    }

    to_file(fout, F);
    // print_map(F);
    return;
}


bool step(const std::string &goal, int pos, std::string q, std::stack<char> stack)
{
    // std::cout << q << " " << goal[pos] << " " << stack.top() << std::endl;
    if (q == "q2")
        return true;
    state tmp = {q, goal[pos], stack.top()};
    if (F.count(tmp) == 0 || pos >= goal.size())
        return false;
    else {
        stack.pop();
        auto tmpStack = stack;
        for (const auto &b : F[tmp]) {
            q = b.q;
            for (int i = b.in.size() - 1; i >= 0; --i) {
                if (b.in[i] != '#')
                    stack.push(b.in[i]);
            }
            if (step(goal, pos + 1, q, stack)) {
                return true;
            }
            stack = tmpStack;
        }
        return false;
    }
}
// 判断是否符合语法
bool is_legal(std::fstream &fin)
{
    std::stack<char> stack;
    stack.push('z');
    std::string q = "q0";
    std::string goal;
    fin >> goal;
    int pos = 0;
    return step(goal, pos, q, stack);
}


int main() {
    std::fstream grammar(grammarPath, std::ios_base::in);
    std::fstream chomsky(chomskyPath, std::ios_base::out | std::ios_base::trunc);
    std::fstream greibach(greibachPath, std::ios_base::out | std::ios_base::trunc);
    std::fstream npda(npdaPath, std::ios_base::out | std::ios_base::trunc);
    std::fstream test(testPath, std::ios_base::in);


    if (!grammar.is_open() || !chomsky.is_open() || !greibach.is_open() || !npda.is_open())
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
            // for (auto val : P[line[0]]) {
            //     std::cout << val << " ";
            // }
            // std::cout << std::endl << std::endl;
        }

        auto P_backup = P;
        auto V_backup = V;
        to_chomsky(chomsky);

        P = P_backup;
        V = V_backup;
        to_greibach(greibach);

        to_NPDA(npda);

        if (is_legal(test))
            std::cout << "Accepted" << std::endl;
        else 
            std::cout << "Unaccepted" << std::endl;

    }
    return 0;
}