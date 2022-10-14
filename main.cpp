#include <iostream>
#include <cassert>
#include <fstream>
#include <set>
#include <string>
#include <cctype>
#include <vector>
#include <map>

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


///////////////////////////////////////////



const char *grammarPath = "./grammar.txt";
const char *chomskyPath = "./Chomsky.txt";
const char *greibachPath = "./Greibach.txt";

std::set<char> V, T;
std::map<char, std::vector<std::string>> P;
char start = 'S';

/**
 *  @brief  translate the common grammar to chomsky normal forms.
 *  @param fin the file inlcuding the grammar
 *  @param fout the file to save chomsky normal forms
 */
void to_chomsky(std::fstream &fin, std::fstream &fout)
{
    std::string s;
    while (fin >> s) {

        for (int i = 0; i < s.size(); ++i) { // 构造非终结符集V和终结符集T
            if (i != 1 && i != 2 && s[i] != '|') {
                if (std::isupper(s[i]))
                    V.insert(s[i]);
                else
                    T.insert(s[i]);
            }
        }

        // 拆分，例如'S->aABC|a' 变为 'S aABC a' ，存储
        P.insert({s[0], std::vector<std::string>()});
        int preind = 3, ind = 3;
        std::cout << s << std::endl;
        while ((ind = s.find_first_of("|", ind, s.size() - ind)) != std::string::npos) {
            P[s[0]].push_back(std::string(s, preind, ind - preind));
            preind = ++ind;
        }
        P[s[0]].push_back(std::string(s, preind, s.size() - preind));
        for (auto val : P[s[0]]) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // 消除无用符号
    

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