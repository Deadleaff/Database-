#include "header2.h"

// ========== РЕАЛИЗАЦИЯ STUDENT ==========

Student::Student(std::string name, int group, double rating)
    : name(name), group(group), rating(rating)
{
    if(name.empty()) {
        throw std::invalid_argument("Некорректное имя");
    }
    if(rating < 2 || rating > 5) {
        throw std::invalid_argument("Некорректный рейтинг (должен быть 2-5)");
    }
    if(group < 0 || group > 999) {
        throw std::invalid_argument("Некорректный номер группы");
    }
}

bool Student::operator<(const Student& other) const {
    return name < other.name;
}

bool Student::operator>(const Student& other) const {
    return name > other.name;
}

// ========== РЕАЛИЗАЦИЯ BTreeNode ==========

BTreeNode::BTreeNode(int _t, bool _leaf) : t(_t), leaf(_leaf) {
    keys.reserve(2*t - 1);
    children.reserve(2*t);
}

void BTreeNode::insertNonFull(Student* student) {
    int i = keys.size() - 1;
    
    if (leaf) {
        keys.push_back(NULL);
        while (i >= 0 && *student < *keys[i]) {
            keys[i + 1] = keys[i];
            i--;
        }
        keys[i + 1] = student;
    } else {
        while (i >= 0 && *student < *keys[i]) {
            i--;
        }
        i++;
        
        if (children[i]->keys.size() == 2*t - 1) {
            splitChild(i, children[i]);
            
            if (*student > *keys[i]) {
                i++;
            }
        }
        children[i]->insertNonFull(student);
    }
    
}

void BTreeNode::splitChild(int i, BTreeNode* y) {
    // Создаем новый узел
    BTreeNode* z = new BTreeNode(y->t, y->leaf);
    
    // СОХРАНЯЕМ средний ключ ДО изменения y
    Student* middleKey = y->keys[t - 1];
    
    // 1. Копируем правую половину ключей из y в z
    for (int j = 0; j < t - 1; j++) {
        z->keys.push_back(y->keys[j + t]);
    }
    
    // 2. Копируем правую половину детей из y в z (если не лист)
    if (!y->leaf) {
        for (int j = 0; j < t; j++) {
            z->children.push_back(y->children[j + t]);
        }
    }
    
    // 3. Уменьшаем y (после копирования)
    y->keys.resize(t - 1);
    if (!y->leaf) {
        y->children.resize(t);
    }
    
    // 4. Вставляем средний ключ в родителя (текущий узел)
    //    БЕЗОПАСНАЯ вставка с проверкой границ
    if (i <= (int)keys.size()) {
        keys.insert(keys.begin() + i, middleKey);
    } else {
        keys.push_back(middleKey);
    }
    
    // 5. Вставляем нового ребенка z
    //    БЕЗОПАСНАЯ вставка с проверкой границ
    if (i + 1 <= (int)children.size()) {
        children.insert(children.begin() + i + 1, z);
    } else {
        children.push_back(z);
    }
}

Student* BTreeNode::search(const std::string& name) {
    int i = 0;
    while (i < (int)keys.size() && name > keys[i]->name) {
        i++;
    }
    
    if (i < (int)keys.size() && name == keys[i]->name) {
        return keys[i];
    }
    
    if (leaf) {
        return NULL;
    }
    
    return children[i]->search(name);
}

void BTreeNode::remove(const std::string& name) {
    int idx = 0;
    while (idx < (int)keys.size() && name > keys[idx]->name) {
        idx++;
    }
    
    if (idx < (int)keys.size() && name == keys[idx]->name) {
        if (leaf) {
            removeFromLeaf(idx);
        } else {
            removeFromNonLeaf(idx);
        }
    } else {
        if (leaf) return;
        
        bool flag = (idx == (int)keys.size());
        
        if (children[idx]->keys.size() < (size_t)t) {
            fill(idx);
        }
        
        if (flag && idx > (int)keys.size()) {
            children[idx - 1]->remove(name);
        } else {
            children[idx]->remove(name);
        }
    }
}

void BTreeNode::removeFromLeaf(int idx) {
    keys.erase(keys.begin() + idx);
}

void BTreeNode::removeFromNonLeaf(int idx) {
    Student* student = keys[idx];
    
    if (children[idx]->keys.size() >= (size_t)t) {
        Student* pred = getPredecessor(idx);
        keys[idx] = pred;
        children[idx]->remove(pred->name);
    }
    else if (children[idx + 1]->keys.size() >= (size_t)t) {
        Student* succ = getSuccessor(idx);
        keys[idx] = succ;
        children[idx + 1]->remove(succ->name);
    }
    else {
        merge(idx);
        children[idx]->remove(student->name);
    }
}

Student* BTreeNode::getPredecessor(int idx) {
    BTreeNode* cur = children[idx];
    while (!cur->leaf) {
        cur = cur->children.back();
    }
    return cur->keys.back();
}

Student* BTreeNode::getSuccessor(int idx) {
    BTreeNode* cur = children[idx + 1];
    while (!cur->leaf) {
        cur = cur->children.front();
    }
    return cur->keys.front();
}

void BTreeNode::fill(int idx) {
    if (idx != 0 && children[idx - 1]->keys.size() >= (size_t)t) {
        borrowFromPrev(idx);
    }
    else if (idx != (int)keys.size() && children[idx + 1]->keys.size() >= (size_t)t) {
        borrowFromNext(idx);
    }
    else {
        if (idx != (int)keys.size()) {
            merge(idx);
        } else {
            merge(idx - 1);
        }
    }
}

void BTreeNode::borrowFromPrev(int idx) {
    BTreeNode* child = children[idx];
    BTreeNode* sibling = children[idx - 1];
    
    child->keys.insert(child->keys.begin(), keys[idx - 1]);
    
    if (!child->leaf) {
        child->children.insert(child->children.begin(), sibling->children.back());
        sibling->children.pop_back();
    }
    
    keys[idx - 1] = sibling->keys.back();
    sibling->keys.pop_back();
}

void BTreeNode::borrowFromNext(int idx) {
    BTreeNode* child = children[idx];
    BTreeNode* sibling = children[idx + 1];
    
    child->keys.push_back(keys[idx]);
    
    if (!child->leaf) {
        child->children.push_back(sibling->children.front());
        sibling->children.erase(sibling->children.begin());
    }
    
    keys[idx] = sibling->keys.front();
    sibling->keys.erase(sibling->keys.begin());
}

void BTreeNode::merge(int idx) {
    BTreeNode* child = children[idx];
    BTreeNode* sibling = children[idx + 1];
    
    child->keys.push_back(keys[idx]);
    
    for (size_t j = 0; j < sibling->keys.size(); j++) {
        child->keys.push_back(sibling->keys[j]);
    }
    
    if (!child->leaf) {
        for (size_t j = 0; j < sibling->children.size(); j++) {
            child->children.push_back(sibling->children[j]);
        }
    }
    
    keys.erase(keys.begin() + idx);
    children.erase(children.begin() + idx + 1);
    
}

BTreeNode::~BTreeNode() {
/*    for (size_t i = 0; i < keys.size(); i++) {
        delete keys[i];
    }*/
    for (size_t i = 0; i < children.size(); i++) {
        delete children[i];
    }
}

// ========== РЕАЛИЗАЦИЯ BTree ==========

BTree::BTree(int _t) : root(NULL), t(_t) {}

void BTree::insert(Student* student) {
    if (root == NULL) {
        root = new BTreeNode(t, true);
        root->keys.push_back(student);
    } else {
        if (root->keys.size() == (size_t)(2*t - 1)) {
            BTreeNode* newRoot = new BTreeNode(t, false);
            newRoot->children.push_back(root);
            newRoot->splitChild(0, root);
            
            int i = 0;
            if (*student > *(newRoot->keys[0])) {
                i++;
            }
            newRoot->children[i]->insertNonFull(student);
            
            root = newRoot;
        } else {
            root->insertNonFull(student);
        }
    }
}

Student* BTree::search(const std::string& name) {
    if (root == NULL) return NULL;
    return root->search(name);
}

void BTree::remove(const std::string& name) {
    if (root == NULL) return;
    
    root->remove(name);
    
    if (root->keys.empty()) {
        BTreeNode* tmp = root;
        if (root->leaf) {
            root = NULL;
        } else {
            root = root->children[0];
        }
       // delete tmp;
    }
}

bool BTree::empty() {
    return root == NULL;
}

BTree::~BTree() {
    delete root;
}

// ========== РЕАЛИЗАЦИЯ GroupList ==========

bool compareStudents(Student* a, Student* b) {
    return a->rating > b->rating;
}

GroupList::GroupList(int num) : group_number(num) {}

int GroupList::get_number() {
    return group_number;
}

void GroupList::add_student(Student* student) {
    students.push_back(student);
}

void GroupList::sortByRating() {
    students.sort(compareStudents);
}

void GroupList::DeleteStudent(Student* s) {
    if(s == NULL) return;
    students.remove(s);
    delete s;
    sortByRating();
}

bool GroupList::deleteStudentByName(const std::string& name) {
    std::list<Student*>::iterator it;
    for (it = students.begin(); it != students.end(); ++it) {
        if ((*it)->name == name) {
            Student* s = *it;
            students.erase(it);
            delete s;
            sortByRating();
            return true;
        }
    }
    return false;
}

Student* GroupList::findStudentByRating(double rating) {
    std::list<Student*>::iterator it;
    for (it = students.begin(); it != students.end(); ++it) {
        if ((*it)->rating == rating) {
            return *it;
        }
    }
    return NULL;
}

std::list<Student*>& GroupList::getStudents() {
    return students;
}

GroupList::~GroupList() {
    std::list<Student*>::iterator it;
    for (it = students.begin(); it != students.end(); ++it) {
        delete *it;
    }
}

// ========== РЕАЛИЗАЦИЯ GroupHashTable ==========

GroupHashTable::GroupHashTable() {
    for (int i = 0; i < 1000; i++) {
        groups[i] = NULL;
    }
}

GroupList* GroupHashTable::get_or_create_list(int num) {
    if (num < 0 || num >= 1000) return NULL;
    if (groups[num] == NULL) {
        groups[num] = new GroupList(num);
    }
    return groups[num];
}

GroupList* GroupHashTable::get_list(int num) {
    if (num < 0 || num >= 1000) return NULL;
    return groups[num];
}

Student* GroupHashTable::findStudentByRating(double rating) {
    for (int i = 0; i < 1000; i++) {
        if (groups[i] != NULL) {
            Student* s = groups[i]->findStudentByRating(rating);
            if (s != NULL) {
                return s;
            }
        }
    }
    return NULL;
}


Student* GroupHashTable::findStudentByRating_group(double rating, int group) {
    if (groups[group] != NULL) {
        Student* s = groups[group]->findStudentByRating(rating);
        if (s != NULL) return s;
    }
    return NULL;
}


GroupHashTable::~GroupHashTable() {
    for (int i = 0; i < 1000; i++) {
        delete groups[i];
    }
}

// ========== РЕАЛИЗАЦИЯ TreeHashTable ==========

TreeHashTable::TreeHashTable() {
    for (int i = 0; i < 1000; i++) {
        trees[i] = NULL;
    }
}

BTree* TreeHashTable::get_or_create_tree(int num) {
    if (num < 0 || num >= 1000) return NULL;
    if (trees[num] == NULL) {
        trees[num] = new BTree(T);
    }
    return trees[num];
}

BTree* TreeHashTable::get_tree(int num) {
    if (num < 0 || num >= 1000) return NULL;
    return trees[num];
}

Student* TreeHashTable::searchByName(int groupNum, const std::string& name) {
    BTree* tree = get_tree(groupNum);
    if (tree != NULL) {
        return tree->search(name);
    }
    return NULL;
}

Student* TreeHashTable::searchByNameAll(const std::string& name) {
    for (int i = 0; i < 1000; i++) {
        if (trees[i] != NULL) {
            Student* s = trees[i]->search(name);
            if (s != NULL) {
                return s;
            }
        }
    }
    return NULL;
}

bool TreeHashTable::delete_from_tree(int groupNum, const std::string& name) {
    BTree* tree = get_tree(groupNum);
    if (tree != NULL) {
        Student* s = tree->search(name);
        if (s != NULL) {
            tree->remove(name);
            return true;
        }
    }
    return false;
}

TreeHashTable::~TreeHashTable() {
    for (int i = 0; i < 1000; i++) {
        delete trees[i];
    }
}

// ========== РЕАЛИЗАЦИЯ DataManager ==========

DataManager::DataManager(GroupHashTable& lists, TreeHashTable& trees) 
    : listTable(lists), treeTable(trees) {}

void DataManager::load(const std::string& filename) {
    std::ifstream file(filename.c_str());

    if (!file.is_open()) {
        throw std::runtime_error("Файл не найден: " + filename);
    }

    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string name, gStr, rStr;

        if (std::getline(ss, name, ';') &&
            std::getline(ss, gStr, ';') &&
            std::getline(ss, rStr)) {

            try {
                size_t first = name.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    name = name.substr(first);
                }
                size_t last = name.find_last_not_of(" \t");
                if (last != std::string::npos) {
                    name = name.substr(0, last + 1);
                }
                
                int gNum = atoi(gStr.c_str());
                double rating = atof(rStr.c_str());

                Student* newStudent = new Student(name, gNum, rating);
                
                GroupList* group = listTable.get_or_create_list(gNum);
                group->add_student(newStudent);
                
                BTree* tree = treeTable.get_or_create_tree(gNum);
                tree->insert(newStudent);
            }
            catch (const std::exception& e) {
                // пропускаем ошибочные строки
            }
        }
    }
    file.close();
}

bool DataManager::deleteStudent(const std::string& name) {
    Student* student = treeTable.searchByNameAll(name);
    
    if (student == NULL) {
        return false;
    }
    
    int groupNum = student->group;
    
    bool treeDeleted = treeTable.delete_from_tree(groupNum, name);
    
    GroupList* group = listTable.get_list(groupNum);
    bool listDeleted = false;
    if (group != NULL) {
        listDeleted = group->deleteStudentByName(name);
    }
    
    return treeDeleted && listDeleted;
}

bool DataManager::deleteStudentFromGroup(int groupNum, const std::string& name) {
    bool treeDeleted = treeTable.delete_from_tree(groupNum, name);
    
    GroupList* group = listTable.get_list(groupNum);
    bool listDeleted = false;
    if (group != NULL) {
        listDeleted = group->deleteStudentByName(name);
    }
    
    return treeDeleted && listDeleted;
}

Student* DataManager::findStudentByName(const std::string& name) {
    return treeTable.searchByNameAll(name);
}

Student* DataManager::findStudentByRating(double rating) {
    return listTable.findStudentByRating(rating);
}

Student* DataManager::findStudentByName_group(const std::string& name, int group) {
    if(treeTable.searchByName(group, name) == NULL) {throw std::invalid_argument("Студент не найден"); return NULL; }
    else return treeTable.searchByName(group, name);
}

Student* DataManager::findStudentByRating_group(double rating, int group) {
    return listTable.findStudentByRating_group(rating, group);

}

