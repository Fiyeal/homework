#include <iostream>
#include <cassert>
#include <fstream>
#include <set>
#include <string>

const char *grammarPath = "./grammar.txt";
const char *chomskyPath = "./Chomsky.txt";
const char *greibachPath = "./Greibach.txt";

std::set<char> V, T;
std::set<std::string> P;
char start = 'S';

/**
 *  @brief  translate the common grammar to chomsky normal forms.
 *  @param fin the file inlcuding the grammar
 *  @param fout the file to save chomsky normal forms
 */
void to_chomsky(std::fstream &fin, std::fstream &fout)
{
    std::string s;
    while (std::getline(fin, s)) {
        fout << s << std::endl;
    }
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