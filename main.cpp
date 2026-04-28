#include "header2.h"

int main() {
    GroupHashTable list_table;
    TreeHashTable tree_table;
    
    try {
        DataManager dm(list_table, tree_table);
        dm.load("dbfile.txt");
        std::string r;
        while(true) {
            std::getline(std::cin, r);
            if(r == "q") { break; }
            else dm.do_request(r);
            dm.rewriteFile("dbfile.txt");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
