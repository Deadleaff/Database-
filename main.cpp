#include "header2.h"

int main() {
    GroupHashTable list_table;
    TreeHashTable tree_table;
    
    try {
        DataManager dm(list_table, tree_table);
        
        // Загрузка данных из файла
        dm.load("dbfile.txt");
        
        // Поиск студента по рейтингу
        Student* a = dm.findStudentByRating(4.5);
        if (a != nullptr) {
            std::cout << "Найден студент с рейтингом 4.5: " << a->name << std::endl;
        }
        
        // Удаление конкретного студента по имени
        dm.deleteStudent("Pavel Ivanov");
        
        // Поиск студентов по подстроке в группе 110
        std::vector<Student*> found = dm.findStudentsBySubstringInGroup("Ily", 110);
        std::cout << "Найдено студентов с 'Iva' в группе 110: " << found.size() << std::endl;
        
        // Удаление студентов по подстроке в группе 110
        int deleted = dm.deleteStudentsBySubstringFromGroup("Ily", 110);
        std::cout << "Удалено студентов: " << deleted << std::endl;
        
        // Сохранение изменений в файл
        dm.rewriteFile("dbfile.txt");
        
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
