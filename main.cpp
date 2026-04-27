#include "header2.h"

int main() {
    GroupHashTable list_table;
    TreeHashTable tree_table;
    
    try {
        DataManager dm(list_table, tree_table);
        dm.load("dbfile.txt");
        std::string r = "SELECT name=Dmi*, group=310, rating=2.5-4.9";
        dm.do_request(r);
        
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
