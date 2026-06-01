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

bool Student::operator>=(double r) {
    return rating >= r;
}

bool Student::operator<=(double r) {
    return rating <= r;
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
    for (int i = 0; i < MAX_GROUP; i++) {
        groups[i] = NULL;
    }
}

GroupList* GroupHashTable::get_or_create_list(int num) {
    if (num < 0 || num >= MAX_GROUP) return NULL;
    if (groups[num] == NULL) {
        groups[num] = new GroupList(num);
    }
    return groups[num];
}

GroupList* GroupHashTable::get_list(int num) {
    if (num < 0 || num >= MAX_GROUP) return NULL;
    return groups[num];
}

Student* GroupHashTable::findStudentByRating(double rating) {
    for (int i = 0; i < MAX_GROUP; i++) {
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
    for (int i = 0; i < MAX_GROUP; i++) {
        delete groups[i];
    }
}

// ========== РЕАЛИЗАЦИЯ TreeHashTable ==========

TreeHashTable::TreeHashTable() {
    for (int i = 0; i < MAX_GROUP; i++) {
        trees[i] = NULL;
    }
}

BTree* TreeHashTable::get_or_create_tree(int num) {
    if (num < 0 || num >= MAX_GROUP) return NULL;
    if (trees[num] == NULL) {
        trees[num] = new BTree(T);
    }
    return trees[num];
}

BTree* TreeHashTable::get_tree(int num) {
    if (num < 0 || num >= MAX_GROUP) return NULL;
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
    for (int i = 0; i < MAX_GROUP; i++) {
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
    for (int i = 0; i < MAX_GROUP; i++) {
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


// ========== РЕАЛИЗАЦИЯ НОВЫХ МЕТОДОВ BTreeNode ==========

void BTreeNode::searchBySubstring(const std::string& substr, std::vector<Student*>& results) {
    // Проверяем все ключи в текущем узле
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i]->name.find(substr) != std::string::npos) {
            results.push_back(keys[i]);
        }
    }

    // Рекурсивно обходим детей
    if (!leaf) {
        for (size_t i = 0; i < children.size(); i++) {
            children[i]->searchBySubstring(substr, results);
        }
    }
}

// ========== РЕАЛИЗАЦИЯ НОВЫХ МЕТОДОВ BTree ==========

std::vector<Student*> BTree::searchBySubstring(const std::string& substr) {
    std::vector<Student*> results;
    if (root != NULL) {
        root->searchBySubstring(substr, results);
    }
    return results;
}

// ========== РЕАЛИЗАЦИЯ НОВЫХ МЕТОДОВ TreeHashTable ==========

std::vector<Student*> TreeHashTable::searchBySubstringInGroup(int groupNum, const std::string& substr) {
    std::vector<Student*> results;
    BTree* tree = get_tree(groupNum);
    if (tree != NULL) {
        results = tree->searchBySubstring(substr);
    }
    return results;
}

bool TreeHashTable::deleteBySubstringFromGroup(int groupNum, const std::string& substr, GroupHashTable& listTable) {
    // Находим всех студентов в группе, у которых в имени есть подстрока
    std::vector<Student*> toDelete = searchBySubstringInGroup(groupNum, substr);

    if (toDelete.empty()) {
        return false;
    }

    // СОБИРАЕМ ИМЕНА ДО УДАЛЕНИЯ (пока указатели еще валидны)
    std::vector<std::string> namesToDelete;
    for (size_t i = 0; i < toDelete.size(); i++) {
        namesToDelete.push_back(toDelete[i]->name);
    }

    // Получаем список группы для удаления (освобождения памяти)
    GroupList* group = listTable.get_list(groupNum);

    // Удаляем каждого найденного студента по имени
    for (size_t i = 0; i < namesToDelete.size(); i++) {
        // Удаляем из B-дерева
        BTree* tree = get_tree(groupNum);
        if (tree != NULL) {
            tree->remove(namesToDelete[i]);
        }

        // Удаляем из списка (освобождает память)
        if (group != NULL) {
            group->deleteStudentByName(namesToDelete[i]);
        }
    }

    return true;
}

// ========== РЕАЛИЗАЦИЯ НОВЫХ МЕТОДОВ DataManager ==========
// ========== РЕАЛИЗАЦИЯ НОВОГО МЕТОДА DataManager ==========

void DataManager::rewriteFile(const std::string& filename) {
    // Открываем файл для записи (это автоматически очистит его содержимое)
    std::ofstream file(filename.c_str(), std::ios::trunc);

    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для записи: " + filename);
    }

    // Проходим по всем группам
    for (int i = 0; i < MAX_GROUP; i++) {
        GroupList* group = listTable.get_list(i);
        if (group != NULL) {
            // Получаем список студентов группы
            std::list<Student*>& students = group->getStudents();

            // Записываем каждого студента в файл, используя итераторы
            for (std::list<Student*>::iterator it = students.begin(); it != students.end(); ++it) {
                Student* s = *it;
                file << s->name << ";" << s->group << ";" << s->rating << std::endl;
            }
        }
    }

    file.close();
}

void TreeHashTable::debug_print_tree(int groupNum) {
    BTree* tree = get_tree(groupNum);
    if (tree == NULL) {
        std::cout << "Дерево для группы " << groupNum << " не существует" << std::endl;
        return;
    }
    std::cout << "Дерево для группы " << groupNum << " существует" << std::endl;
}

// ========== РЕАЛИЗАЦИЯ МЕТОДОВ ПАРСИНГА DataManager ==========

std::string DataManager::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    if (last == std::string::npos) return "";
    return str.substr(first, last - first + 1);
}

void DataManager::parseGroup(const std::string& token, United& info) {
    std::string groupStr = token;
    size_t eqPos = groupStr.find('=');
    if (eqPos != std::string::npos) {
        if (eqPos + 1 < groupStr.length()) {
            groupStr = groupStr.substr(eqPos + 1);
        } else {
            return;
        }
    }
    groupStr = trim(groupStr);
    if (groupStr.empty()) return;
    
    size_t dashPos = groupStr.find('-');
    if (dashPos != std::string::npos) {
        int start = atoi(trim(groupStr.substr(0, dashPos)).c_str());
        int end = atoi(trim(groupStr.substr(dashPos + 1)).c_str());
        info.has_group_filter = true;
        info.group_nums.clear();
        for (int g = start; g <= end; g++) {
            info.group_nums.push_back(g);
        }
    } else {
        int group = atoi(groupStr.c_str());
        info.has_group_filter = true;
        info.group_nums.clear();
        info.group_nums.push_back(group);
    }
}

void DataManager::parseRating(const std::string& token, United& info) {
    std::string ratingStr = token;

    // Убираем "=" если есть
    size_t eqPos = ratingStr.find('=');
    if (eqPos != std::string::npos) {
        if (eqPos + 1 < ratingStr.length()) {
            ratingStr = ratingStr.substr(eqPos + 1);
        } else {
            return;
        }
    }

    ratingStr = trim(ratingStr);
    if (ratingStr.empty()) return;

    // Ищем тире (поддерживаем пробелы вокруг)
    size_t dashPos = ratingStr.find('-');
    if (dashPos != std::string::npos) {
        std::string startStr = trim(ratingStr.substr(0, dashPos));
        std::string endStr = trim(ratingStr.substr(dashPos + 1));

        if (startStr.empty() || endStr.empty()) return;

        info.min_rating = atof(startStr.c_str());
        info.max_rating = atof(endStr.c_str());
        info.has_rating_filter = true;

    } else {
        info.min_rating = info.max_rating = atof(ratingStr.c_str());
        info.has_rating_filter = true;
    }
}

// ============================================================
// СЕРВЕРНАЯ ВЕРСИЯ execute_request
// ============================================================

Result DataManager::execute_request(const std::string& req, ClientSession* session) {
    United info;
    std::string trimmed_req = trim(req);
    std::stringstream ss(trimmed_req);
    std::string token;

    // Считываем команду
    ss >> token;
    std::string commandStr = token;
    std::transform(commandStr.begin(), commandStr.end(), commandStr.begin(), ::toupper);

    // ============================================================
    // 1. КОМАНДА PRINT (не меняет состояние БД)
    // ============================================================
    if (commandStr == "PRINT") {
        // Проверяем, была ли успешная команда SELECT ранее
        if (session == nullptr || !session->has_last_select || session->last_result == nullptr) {
            return Result(ERROR_PRINT_DIDNOT_SELECT, 
                "PRINT: нет данных для печати. Сначала выполните SELECT.");
        }
        
        // Парсим список колонок, если он указан
        std::vector<std::string> columns;
        std::string columns_line;
        
        // Читаем остаток строки после PRINT
        if (std::getline(ss, columns_line)) {
            columns_line = trim(columns_line);
            if (!columns_line.empty()) {
                // Разбираем колонки, разделённые запятыми
                std::stringstream col_ss(columns_line);
                std::string col;
                while (std::getline(col_ss, col, ',')) {
                    col = trim(col);
                    if (!col.empty()) {
                        columns.push_back(col);
                    }
                }
            }
        }
        
        // Если колонки не указаны, выводим все стандартные
        if (columns.empty()) {
            columns.push_back("name");
            columns.push_back("group");
            columns.push_back("rating");
        }
        
        // Валидация колонок
        for (const auto& col : columns) {
            if (col != "name" && col != "group" && col != "rating") {
                return Result(ERROR_PRINT_INVALID_COLUMN, 
                    "PRINT: неизвестная колонка '" + col + "'. Доступные колонки: name, group, rating");
            }
        }
        
        // Создаём результат с данными из последнего SELECT
        Result result;
        result.error_code = SUCCESS;
        result.students = session->last_result->students;
        result.columns = columns;
        
        return result;
    }

    // ============================================================
    // 2. ПАРСИНГ ПАРАМЕТРОВ ДЛЯ ДРУГИХ КОМАНД
    // ============================================================
    
    // Сбрасываем флаги для новой команды
    info = United();
    
    // Парсим параметры
    while (ss >> token) {
        if (!token.empty() && token.back() == ',') {
            token.pop_back();
        }

        std::string lower = token;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // --- Обработка name (может содержать пробелы) ---
        if (lower == "name") {
            ss >> token;
            if (token == "=") {
                ss >> token;
            }

            std::string nameValue;
            bool firstToken = true;

            while (true) {
                std::string nextLower = token;
                std::transform(nextLower.begin(), nextLower.end(), nextLower.begin(), ::tolower);

                if (nextLower == "group" || nextLower == "rating") {
                    ss << " " << token;
                    break;
                }

                if (!token.empty() && token.back() == ',') {
                    token.pop_back();
                    if (!firstToken) nameValue += " ";
                    nameValue += token;
                    break;
                }

                if (!firstToken) nameValue += " ";
                nameValue += token;
                firstToken = false;

                if (!(ss >> token)) break;
            }

            nameValue = trim(nameValue);
            info.has_subname = true;
            if (!nameValue.empty() && nameValue.back() == '*') {
                info.subname = nameValue.substr(0, nameValue.length() - 1);
            } else {
                info.subname = nameValue;
            }
        }
        // --- Обработка group ---
        else if (lower == "group") {
            ss >> token;
            if (token == "=") {
                ss >> token;
            }
            if (!token.empty() && token.back() == ',') token.pop_back();
            parseGroup(token, info);
            info.has_group_filter = true;
        }
        // --- Обработка rating ---
        else if (lower == "rating") {
            ss >> token;
            if (token == "=") {
                ss >> token;
            }

            std::string ratingValue = token;

            // Проверяем, есть ли тире (следующий токен)
            std::string next_token;
            if (ss >> next_token && next_token == "-") {
                ss >> next_token;
                ratingValue += "-" + next_token;
            }

            parseRating(ratingValue, info);
            info.has_rating_filter = true;
        }
    }

    // ============================================================
    // 3. ВЫПОЛНЕНИЕ КОМАНДЫ
    // ============================================================
    
    Result result;
    
    if (commandStr == "SELECT") {
        // Заполняем значения по умолчанию
        if (!info.has_group_filter) {
            for (int i = 0; i < MAX_GROUP; i++) {
                info.group_nums.push_back(i);
            }
        }
        if (!info.has_rating_filter) {
            info.min_rating = 2.0;
            info.max_rating = 5.0;
        }
        
        result = handle_select(info);
        
        // Сохраняем результат в сессии для последующего PRINT
        if (session && result.is_success()) {
            if (session->last_result != nullptr) {
                delete session->last_result;
            }
            session->last_result = new Result(result);
            session->has_last_select = true;
        }
    }
    else if (commandStr == "INSERT") {
        result = handle_insert(info);
    }
    else if (commandStr == "REMOVE") {
        result = handle_remove(info);
    }
    else {
        result = Result(ERROR_UNKNOWN_COMMAND, "Неизвестная команда: " + commandStr);
    }
    
    return result;
}

// ============================================================
// HANDLE_SELECT
// ============================================================

Result DataManager::handle_select(const United& info) {
    std::vector<Student*> results;

    if (info.has_subname && !info.subname.empty()) {
        // Поиск по подстроке
        for (size_t i = 0; i < info.group_nums.size(); i++) {
            if (info.group_nums[i] < 0 || info.group_nums[i] >= MAX_GROUP) {
                continue;
            }
            std::vector<Student*> found = treeTable.searchBySubstringInGroup(
                info.group_nums[i], info.subname);
            for (size_t j = 0; j < found.size(); j++) {
                if (found[j]->rating >= info.min_rating &&
                    found[j]->rating <= info.max_rating) {
                    results.push_back(found[j]);
                }
            }
        }
    } else {
        // Поиск по рейтингу
        for (size_t i = 0; i < info.group_nums.size(); i++) {
            int groupNum = info.group_nums[i];
            if (groupNum < 0 || groupNum >= MAX_GROUP) continue;

            GroupList* group = listTable.get_list(groupNum);
            if (group != NULL) {
                std::list<Student*>& students = group->getStudents();
                for (auto it = students.begin(); it != students.end(); ++it) {
                    if ((*it)->rating >= info.min_rating &&
                        (*it)->rating <= info.max_rating) {
                        results.push_back(*it);
                    }
                }
            }
        }
    }

    return Result(results);
}

// ============================================================
// HANDLE_INSERT
// ============================================================

Result DataManager::handle_insert(const United& info) {
    // Проверки
    if (!info.has_subname || info.subname.empty()) {
        return Result(ERROR_INVALID_NAME, "Не указано имя студента. Используйте: name = Имя");
    }

    if (!info.has_group_filter || info.group_nums.empty()) {
        return Result(ERROR_INVALID_GROUP, "Не указана группа. Используйте: group = N");
    }

    if (!info.has_rating_filter) {
        return Result(ERROR_INVALID_RATING, "Не указан рейтинг. Используйте: rating = X.X");
    }

    int groupNum = info.group_nums[0];
    double rating = info.min_rating;

    if (groupNum < 0 || groupNum >= MAX_GROUP) {
        return Result(ERROR_INVALID_GROUP, "Некорректный номер группы (0-999)");
    }

    if (rating < 2.0 || rating > 5.0) {
        return Result(ERROR_INVALID_RATING, "Некорректный рейтинг (должен быть 2.0-5.0)");
    }

    try {
        Student* newStudent = new Student(info.subname, groupNum, rating);

        GroupList* group = listTable.get_or_create_list(groupNum);
        group->add_student(newStudent);
        group->sortByRating();

        BTree* tree = treeTable.get_or_create_tree(groupNum);
        tree->insert(newStudent);

        std::string msg = "INSERT: успешно добавлен студент \"" + info.subname +
                          "\" (группа " + std::to_string(groupNum) +
                          ", рейтинг " + std::to_string(rating) + ")";
        return Result(msg);
    }
    catch (const std::exception& e) {
        return Result(ERROR_INSERT_FAILED, std::string("INSERT: ошибка - ") + e.what());
    }
}

// ============================================================
// HANDLE_REMOVE
// ============================================================

Result DataManager::handle_remove(const United& info) {
    int removedCount = 0;
    std::vector<std::string> removedNames;

    // Если указано имя с подстрокой
    if (info.has_subname && !info.subname.empty()) {
        std::vector<int> groupsToCheck;
        if (info.has_group_filter && !info.group_nums.empty()) {
            groupsToCheck = info.group_nums;
        } else {
            for (int i = 0; i < MAX_GROUP; i++) {
                if (listTable.get_list(i) != NULL) {
                    groupsToCheck.push_back(i);
                }
            }
        }

        for (size_t i = 0; i < groupsToCheck.size(); i++) {
            int groupNum = groupsToCheck[i];
            std::vector<Student*> found = treeTable.searchBySubstringInGroup(groupNum, info.subname);

            std::vector<Student*> toDelete;
            for (size_t j = 0; j < found.size(); j++) {
                if (info.has_rating_filter) {
                    if (found[j]->rating >= info.min_rating &&
                        found[j]->rating <= info.max_rating) {
                        toDelete.push_back(found[j]);
                    }
                } else {
                    toDelete.push_back(found[j]);
                }
            }

            for (size_t j = 0; j < toDelete.size(); j++) {
                removedNames.push_back(toDelete[j]->name);
                removedCount++;
            }

            if (toDelete.size() > 0) {
                treeTable.deleteBySubstringFromGroup(groupNum, info.subname, listTable);
            }
        }
    }
    // Если имя не указано, но есть группа или рейтинг
    else if (info.has_group_filter || info.has_rating_filter) {
        std::vector<int> groupsToCheck;
        if (info.has_group_filter && !info.group_nums.empty()) {
            groupsToCheck = info.group_nums;
        } else {
            for (int i = 0; i < MAX_GROUP; i++) {
                if (listTable.get_list(i) != NULL) {
                    groupsToCheck.push_back(i);
                }
            }
        }

        for (size_t i = 0; i < groupsToCheck.size(); i++) {
            int groupNum = groupsToCheck[i];
            GroupList* group = listTable.get_list(groupNum);
            BTree* tree = treeTable.get_tree(groupNum);

            if (group == NULL || tree == NULL) {
                continue;
            }

            std::list<Student*>& students = group->getStudents();
            std::vector<Student*> toDelete;

            for (auto it = students.begin(); it != students.end(); ++it) {
                Student* s = *it;
                bool matches = true;

                if (info.has_rating_filter) {
                    if (s->rating < info.min_rating || s->rating > info.max_rating) {
                        matches = false;
                    }
                }

                if (matches) {
                    toDelete.push_back(s);
                }
            }

            for (size_t j = 0; j < toDelete.size(); j++) {
                Student* s = toDelete[j];
                removedNames.push_back(s->name);
                tree->remove(s->name);
                group->deleteStudentByName(s->name);
                removedCount++;
            }
        }
    }
    else {
        return Result(ERROR_INVALID_NAME, "REMOVE: не указаны условия для удаления");
    }

    if (removedCount == 0) {
        std::string msg = "REMOVE: не найдено студентов для удаления\n";
        if (info.has_subname) {
            msg += "  Поиск по имени: \"" + info.subname + "\"\n";
        }
        if (info.has_group_filter && !info.group_nums.empty()) {
            msg += "  Поиск по группам: " + std::to_string(info.group_nums[0]);
            if (info.group_nums.size() > 1) {
                msg += "-" + std::to_string(info.group_nums.back());
            }
            msg += "\n";
        }
        if (info.has_rating_filter) {
            msg += "  Поиск по рейтингу: " + std::to_string(info.min_rating) +
                   " - " + std::to_string(info.max_rating) + "\n";
        }
        return Result(ERROR_STUDENT_NOT_FOUND, msg);
    }

    std::string msg = "REMOVE: удалено студентов: " + std::to_string(removedCount);
    if (removedCount <= 20) {
        msg += "\nУдаленные студенты:";
        for (size_t i = 0; i < removedNames.size(); i++) {
            msg += "\n  - " + removedNames[i];
        }
    }

    return Result(msg);
}

// ============================================================
// РЕАЛИЗАЦИЯ Result::serialize()
// ============================================================

std::string Result::serialize() {
    std::stringstream ss;

    // Формат: код ошибки
    ss << error_code << "\n";

    if (error_code != SUCCESS) {
        ss << error_message << "\n";
        return ss.str();
    }

    /*if (!students.empty()) {
        // Количество студентов
        ss << students.size() << "\n";
        for (size_t i = 0; i < students.size(); i++) {
            ss << students[i]->name << ";"
               << students[i]->group << ";"
               << students[i]->rating << "\n";
        }*/
    /*} else */if (!message.empty()) {
        ss << message << "\n";
    } else {
        ss << "OK\n";
    }
    std::string ans = ss.str();
    this->message_length = ans.length();

    return ss.str();
}

// ============================================================
// ПАРСИНГ КОЛОНОК ДЛЯ PRINT
// ============================================================

void DataManager::parseColumns(const std::string& token, std::vector<std::string>& columns) {
    std::stringstream ss(token);
    std::string col;
    
    columns.clear();
    
    while (std::getline(ss, col, ',')) {
        col = trim(col);
        if (!col.empty()) {
            columns.push_back(col);
        }
    }
}

// ============================================================
// ФОРМАТИРОВАНИЕ RESULT В ТАБЛИЦУ
// ============================================================

std::string Result::format_as_table(const std::vector<std::string>& columns)  {
    std::stringstream ss;
    
    if (error_code != SUCCESS) {
        ss << "+-----------------------------+\n";
        ss << "|           ОШИБКА            |\n";
        ss << "+-----------------------------+\n";
        ss << "| Код: " << std::setw(24) << error_code << "|\n";
        ss << "| " << std::setw(28) << error_message.substr(0, 28) << "|\n";
        ss << "+-----------------------------+\n";
        return ss.str();
    }
    
    if (!message.empty()) {
        ss << "+-----------------------------+\n";
        ss << "|         СООБЩЕНИЕ           |\n";
        ss << "+-----------------------------+\n";
        std::string msg = message.substr(0, 28);
        ss << "| " << std::setw(28) << msg << "|\n";
        ss << "+-----------------------------+\n";
        return ss.str();
    }
    
    if (students.empty()) {
        ss << "+-----------------------------+\n";
        ss << "|     РЕЗУЛЬТАТ ЗАПРОСА       |\n";
        ss << "+-----------------------------+\n";
        ss << "| Найдено студентов: " << std::setw(15) << "0" << "|\n";
        ss << "+-----------------------------+\n";
        return ss.str();
    }
    
    // Вычисляем ширину колонок
    std::vector<std::string> display_columns = columns;
    if (display_columns.empty()) {
        display_columns.push_back("name");
        display_columns.push_back("group");
        display_columns.push_back("rating");
    }
    
    std::map<std::string, size_t> col_widths;
    for (const auto& col : display_columns) {
        size_t width = col.length();
        for (Student* s : students) {
            size_t val_len;
            if (col == "name") val_len = s->name.length();
            else if (col == "group") val_len = std::to_string(s->group).length();
            else {
                std::stringstream ss_val;
                ss_val << std::fixed << std::setprecision(2) << s->rating;
                val_len = ss_val.str().length();
            }
            if (val_len > width) width = val_len;
        }
        col_widths[col] = width;
    }
    
    // Верхняя граница
    ss << "+";
    for (size_t i = 0; i < display_columns.size(); i++) {
        ss << std::string(col_widths[display_columns[i]] + 2, '-');
        if (i < display_columns.size() - 1) ss << "+";
    }
    ss << "+\n";
    
    // Заголовки
    ss << "|";
    for (const auto& col : display_columns) {
        std::string upper = col;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        ss << " " << std::left << std::setw(col_widths[col]) << upper << " |";
    }
    ss << "\n";
    
    // Разделитель
    ss << "+";
    for (size_t i = 0; i < display_columns.size(); i++) {
        ss << std::string(col_widths[display_columns[i]] + 2, '-');
        if (i < display_columns.size() - 1) ss << "+";
    }
    ss << "+\n";
    
    // Данные
    for (Student* s : students) {
        ss << "|";
        for (const auto& col : display_columns) {
            std::string value;
            if (col == "name") value = s->name;
            else if (col == "group") value = std::to_string(s->group);
            else {
                std::stringstream ss_val;
                ss_val << std::fixed << std::setprecision(2) << s->rating;
                value = ss_val.str();
            }
            ss << " " << std::left << std::setw(col_widths[col]) << value << " |";
        }
        ss << "\n";
    }
    
    // Нижняя граница
    ss << "+";
    for (size_t i = 0; i < display_columns.size(); i++) {
        ss << std::string(col_widths[display_columns[i]] + 2, '-');
        if (i < display_columns.size() - 1) ss << "+";
    }
    ss << "+\n";
    
    ss << "Всего студентов: " << students.size() << "\n";

    std::string ans = ss.str();
    this->message_length = ans.length();
    
    return ss.str();
}

// ============================================================
// HANDLE_PRINT
// ============================================================

Result DataManager::handle_print(const std::vector<std::string>& columns, const Result* last_result) {
    if (last_result == nullptr || last_result->error_code != SUCCESS || last_result->students.empty()) {
        return Result(ERROR_PRINT_DIDNOT_SELECT, "Нет результатов для печати. Сначала выполните SELECT.");
    }
    
    // Проверяем валидность колонок
    for (const auto& col : columns) {
        if (col != "name" && col != "group" && col != "rating") {
            return Result(ERROR_PRINT_INVALID_COLUMN, "Неизвестная колонка: " + col);
        }
    }
    
    Result result;
    result.error_code = SUCCESS;
    result.students = last_result->students;
    result.columns = columns;
    
    return result;
}
