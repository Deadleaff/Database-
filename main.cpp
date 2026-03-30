#include "header2.h"

int main () {
    GroupHashTable list_table;
    TreeHashTable tree_table;
    Student* a = nullptr;
    Student* s = nullptr;
    try {
        
        DataManager dm(list_table, tree_table);

        dm.load("dbfile.txt");

        a = dm.findStudentByRating(4.5);

        s = dm.findStudentByName_group ("Pavel Ivanov", 510);
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
        
    std::cout << s->rating << "\n";
    return 0;
}

