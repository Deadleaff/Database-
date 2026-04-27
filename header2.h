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

typedef enum { SELECT, RESELECT, PRINT, INSERT, REMOVE, UNKNOWN } Commands;

class Request {
public:
    Commands command;
    United info;
    
    Request() : command(UNKNOWN) {}
    Request(Commands cmd, const United& inf) : command(cmd), info(inf) {}
};

// ========== ОБЪЕДИНЕННЫЙ МЕНЕДЖЕР ==========

class DataManager {
private:
    GroupHashTable& listTable;   // таблица списков
    TreeHashTable& treeTable;    // таблица деревьев
    
    // Приватные методы для парсинга (новые)
    std::string trim(const std::string& str);
    void parseGroup(const std::string& token, United& info);
    void parseRating(const std::string& token, United& info);
    
public:
    DataManager(GroupHashTable& lists, TreeHashTable& trees);  // конструктор
    void load(const std::string& filename);                     // загрузка из файла
    void rewriteFile(const std::string& filename);              // Перезаписать файл текущими данными
    void do_request(const std::string& req);                    // выполнить запрос (результат в output.txt)
};

#endif // HEADER1_H
