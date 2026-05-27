#ifndef HEADER1_H
#define HEADER1_H

#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <map>

const int T = 3;  // минимальная степень B-дерева
const int MAX_GROUP = 1000; //максимальное число групп

// ========== СТРУКТУРА STUDENT ==========

struct Student {
    std::string name;   // имя студента
    int group;          // номер группы (0-999)
    double rating;      // рейтинг (2.0-5.0)

    Student(std::string name, int group, double rating);  // конструктор с валидацией
    bool operator<(const Student& other) const;           // сравнение по имени
    bool operator>(const Student& other) const;           // обратное сравнение
    bool operator>=(double r);
    bool operator<=(double r);
};

// ========== B-ДЕРЕВО ==========

class BTreeNode {
public:
    std::vector<Student*> keys;        // ключи (указатели на студентов)
    std::vector<BTreeNode*> children;  // дочерние узлы
    bool leaf;                          // флаг листа
    int t;                               // минимальная степень
    
    BTreeNode(int _t, bool _leaf);                      // конструктор
    void insertNonFull(Student* student);               // вставка в неполный узел
    void splitChild(int i, BTreeNode* y);               // разделение ребенка
    Student* search(const std::string& name);           // поиск по имени
    void remove(const std::string& name);                // удаление
    void removeFromLeaf(int idx);                        // удаление из листа
    void removeFromNonLeaf(int idx);                     // удаление из внутр. узла
    Student* getPredecessor(int idx);                    // предшественник
    Student* getSuccessor(int idx);                      // преемник
    void fill(int idx);                                  // пополнение ребенка
    void borrowFromPrev(int idx);                        // заимствование у левого
    void borrowFromNext(int idx);                        // заимствование у правого
    void searchBySubstring(const std::string& substr, std::vector<Student*>& results);
    void merge(int idx);                                 // объединение детей
    ~BTreeNode();                                         // деструктор
};

class BTree {
private:
    BTreeNode* root;  // корень дерева
    int t;             // минимальная степень
    
public:
    BTree(int _t);                          // конструктор
    void insert(Student* student);           // вставка студента
    Student* search(const std::string& name); // поиск по имени
    void remove(const std::string& name);    // удаление по имени
    std::vector<Student*> searchBySubstring(const std::string& substr);
    bool empty();                            // проверка на пустоту
    ~BTree();                                 // деструктор
};

// ========== СПИСОК ГРУППЫ ==========

bool compareStudents(Student* a, Student* b);  // сравнение по рейтингу

class GroupList {
private:
    int group_number;              // номер группы
    std::list<Student*> students;  // список студентов

public:
    GroupList(int num);                                   // конструктор
    int get_number();                                      // получить номер
    void add_student(Student* student);                    // добавить студента
    void sortByRating();                                   // сортировка по рейтингу
    void DeleteStudent(Student* s);                        // удалить по указателю
    bool deleteStudentByName(const std::string& name);    // удалить по имени
    Student* findStudentByRating(double rating);           // поиск по рейтингу
    std::list<Student*>& getStudents();                    // получить список
    ~GroupList();                                           // деструктор
};

// ========== ХЭШ-ТАБЛИЦА ДЛЯ СПИСКОВ ==========

class GroupHashTable {
private:
    GroupList* groups[MAX_GROUP];  // массив указателей на группы (0-999)

public:
    GroupHashTable();                                               // конструктор
    GroupList* get_or_create_list(int num);                         // получить или создать
    GroupList* get_list(int num);                                   // получить существующую
    Student* findStudentByRating(double rating);                    // поиск студента по рейтингу
    Student* findStudentByRating_group(double rating, int group);    // поиск в группе по рейтингу
    ~GroupHashTable();                                              // деструктор
};

// ========== ХЭШ-ТАБЛИЦА ДЛЯ ДЕРЕВЬЕВ ==========

class TreeHashTable {
private:
    BTree* trees[MAX_GROUP];  // массив указателей на деревья (0-999)

public:
    TreeHashTable();                                        // конструктор
    BTree* get_or_create_tree(int num);                     // получить или создать
    BTree* get_tree(int num);                               // получить существующее
    Student* searchByName(int groupNum, const std::string& name);  // поиск по имени в группе
    Student* searchByNameAll(const std::string& name);              // поиск по имени во всех группах
    std::vector<Student*> searchBySubstringInGroup(int groupNum, const std::string& substr);
    bool deleteBySubstringFromGroup(int groupNum, const std::string& substr, GroupHashTable& listTable);
    bool delete_from_tree(int groupNum, const std::string& name);  // удалить из дерева
    void debug_print_tree(int groupNum);
    ~TreeHashTable();                                       // деструктор
};

// ========== СТРУКТУРЫ ДЛЯ ЗАПРОСОВ ==========

struct United {
    std::vector<int> group_nums;
    double max_rating;
    double min_rating;
    std::string subname;
    bool has_subname;
    bool has_group_filter;
    bool has_rating_filter;
    
    United() : max_rating(5.0), min_rating(2.0), has_subname(false), 
               has_group_filter(false), has_rating_filter(false) {}
    
    United(std::vector<int> groups, double max_r, double min_r, std::string subname) 
        : group_nums(groups), max_rating(max_r), min_rating(min_r), subname(subname),
          has_subname(!subname.empty()), has_group_filter(!groups.empty()),
          has_rating_filter(true) {}
};

struct Result;
struct ClientSession;
// ========== ОБЪЕДИНЕННЫЙ МЕНЕДЖЕР ==========

class DataManager {
private:
    GroupHashTable& listTable;   // таблица списков
    TreeHashTable& treeTable;    // таблица деревьев
    
    // Приватные методы для парсинга (новые)
    std::string trim(const std::string& str);
    void parseGroup(const std::string& token, United& info);
    void parseRating(const std::string& token, United& info);
    void parseColumns(const std::string& token, std::vector<std::string>& columns);

    Result handle_select(const United& info);
    Result handle_insert(const United& info);
    Result handle_remove(const United& info);
    Result handle_print(const std::vector<std::string>& columns, const Result* last_result);
    
public:
    DataManager(GroupHashTable& lists, TreeHashTable& trees);  // конструктор
    void load(const std::string& filename);                     // загрузка из файла
    void rewriteFile(const std::string& filename);              // Перезаписать файл текущими данными
    Result execute_request(const std::string& req, ClientSession* session = nullptr);
};


// ============================================================
// КОДЫ ОШИБОК
// ============================================================

enum ErrorCode {
    SUCCESS = 0,
    ERROR_UNKNOWN_COMMAND = 1,
    ERROR_INVALID_NAME = 2,
    ERROR_INVALID_GROUP = 3,
    ERROR_INVALID_RATING = 4,
    ERROR_STUDENT_NOT_FOUND = 5,
    ERROR_DUPLICATE_STUDENT = 6,
    ERROR_GROUP_NOT_FOUND = 7,
    ERROR_INSERT_FAILED = 8,
    ERROR_REMOVE_FAILED = 9,
    ERROR_PRINT_DIDNOT_SELECT = 10,
    ERROR_PRINT_INVALID_COLUMN = 11,
    ERROR_INTERNAL = 99,
};

// ============================================================
// СТРУКТУРА RESULT
// ============================================================

struct Result {
    int error_code;
    std::string error_message;
    std::vector<Student*> students;
    std::string message;
    std::vector<std::string> columns;  // запрошенные колонки для PRINT
    int message_length;

    Result() : error_code(SUCCESS) {}
    
    Result(const std::vector<Student*>& students)
        : error_code(SUCCESS), students(students) {}
    
    Result(const std::string& msg)
        : error_code(SUCCESS), message(msg) {}
    
    Result(int code, const std::string& msg)
        : error_code(code), error_message(msg) {}
    
    bool is_success() const { return error_code == SUCCESS; }
    
    std::string serialize();
    
    // Форматирование в таблицу
    std::string format_as_table(const std::vector<std::string>& columns) const;
    
    std::string get_error_string() const {
        switch (error_code) {
            case SUCCESS: return "Успех";
            case ERROR_UNKNOWN_COMMAND: return "Неизвестная команда";
            case ERROR_INVALID_NAME: return "Некорректное имя";
            case ERROR_INVALID_GROUP: return "Некорректная группа";
            case ERROR_INVALID_RATING: return "Некорректный рейтинг";
            case ERROR_STUDENT_NOT_FOUND: return "Студент не найден";
            case ERROR_DUPLICATE_STUDENT: return "Дубликат студента";
            case ERROR_GROUP_NOT_FOUND: return "Группа не найдена";
            case ERROR_INSERT_FAILED: return "Ошибка вставки";
            case ERROR_REMOVE_FAILED: return "Ошибка удаления";
            case ERROR_PRINT_DIDNOT_SELECT: return "Печать невозможна: сначала выполните SELECT";
            case ERROR_PRINT_INVALID_COLUMN: return "Неверное имя колонки";
            case ERROR_INTERNAL: return "Внутренняя ошибка";
            default: return "Неизвестная ошибка";
        }
    }
};

// ========== СТРУКТУРА СЕССИИ КЛИЕНТА ==========

struct ClientSession {
    int fd;                         // файловый дескриптор сокета
    Result* last_result;            // последний результат SELECT
    bool has_last_select;           // был ли успешный SELECT
    std::vector<std::string> last_columns; // последние запрошенные колонки

    ClientSession(int fd) : fd(fd), last_result(nullptr), has_last_select(false) {}

    ~ClientSession() {
        if (last_result != nullptr) {
            delete last_result;
        }
    }

    // Запрещаем копирование
    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
};


#endif
