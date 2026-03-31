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

// ========== СТРУКТУРА STUDENT ==========

struct Student {
    std::string name;   // имя студента
    int group;          // номер группы (0-999)
    double rating;      // рейтинг (2.0-5.0)

    Student(std::string name, int group, double rating);  // конструктор с валидацией
    bool operator<(const Student& other) const;           // сравнение по имени
    bool operator>(const Student& other) const;           // обратное сравнение
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
    GroupList* groups[1000];  // массив указателей на группы (0-999)

public:
    GroupHashTable();                                               // конструктор
    GroupList* get_or_create_list(int num);                         // получить или создать
    GroupList* get_list(int num);                                   // получить существующую
    Student* findStudentByRating(double rating);                    // поиск студента по рейтингу
    Student* findStudentByRating_group(double rating, int group);     // поиск в группе по рейтингу
    ~GroupHashTable();                                              // деструктор
};

// ========== ХЭШ-ТАБЛИЦА ДЛЯ ДЕРЕВЬЕВ ==========

class TreeHashTable {
private:
    BTree* trees[1000];  // массив указателей на деревья (0-999)

public:
    TreeHashTable();                                        // конструктор
    BTree* get_or_create_tree(int num);                     // получить или создать
    BTree* get_tree(int num);                               // получить существующее
    Student* searchByName(int groupNum, const std::string& name);  // поиск по имени в группе
    Student* searchByNameAll(const std::string& name);              // поиск по имени во всех группах
    bool delete_from_tree(int groupNum, const std::string& name);  // удалить из дерева
    ~TreeHashTable();                                       // деструктор
};

// ========== ОБЪЕДИНЕННЫЙ МЕНЕДЖЕР ==========

class DataManager {
private:
    GroupHashTable& listTable;   // таблица списков
    TreeHashTable& treeTable;    // таблица деревьев

public:
    DataManager(GroupHashTable& lists, TreeHashTable& trees);  // конструктор
    void load(const std::string& filename);                     // загрузка из файла
    bool deleteStudent(const std::string& name);                // удалить по имени
    bool deleteStudentFromGroup(int groupNum, const std::string& name);  // удалить из группы
    Student* findStudentByName(const std::string& name);        // найти студента по имени (через дерево)
    Student* findStudentByRating(double rating);                 // найти студента по рейтингу (через список)
    Student* findStudentByName_group(const std::string& name, int group); // найти по имени и по группе
    Student* findStudentByRating_group(double rating, int group); // найти по рейтингу и по группе
};

#endif // HEADER1_H
