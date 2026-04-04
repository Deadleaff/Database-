#include "header2.h"

int main () {
    GroupHashTable list_table;
    TreeHashTable tree_table;
    Student* a;
    Student* s = nullptr;
    try {
        
        DataManager dm(list_table, tree_table);

        dm.load("dbfile.txt");

        a = dm.findStudentByRating(4.5);
        dm.deleteStudent("Pavel Ivanov");


        s = dm.findStudentByName_group ("Pavel Ivanov", 210);
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
        
    std::cout << a->rating << "\n";
    return 0;
}

