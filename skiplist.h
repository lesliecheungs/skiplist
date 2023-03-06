#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>
#include <algorithm>

// 存储或读取至文件的路径
#define STORE_FILE "store/dumpFile"

// 对链表进行增减需要锁
std::mutex mtx;
std::string delimiter = ":";

// 定义节点
template<typename K, typename V>
class Node {
public:
    Node(){}
    Node(K k, V v, int);
    ~Node();

    K get_key() const;
    V get_value() const;

    void set_value(V);

    // 线性数组用于保存指向不同级别的下一个节点的指针
    Node<K, V>** forward;

    int node_level;

private:
    K key;
    V value;
};

template<typename K, typename V>
Node<K,V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level;

    this->forward = new Node<K, V>*[level+1];

    memset(this->forward, 0, sizeof(Node<K,V>*) * (level+1));
}

template<typename K, typename V>
Node<K, V>::~Node() {
    delete []forward;
}

template<typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
}

template<typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
}

template<typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value = value;
}

template<typename K, typename V>
class SkipList {
public:
    SkipList(int);
    ~SkipList();

    
    int get_random_level(); // 随机的得到节点最高可以指向第几层
    Node<K, V>* create_node(K, V, int);
    int insert_element(K, V); // 插入元素
    void display_list();    // 图像化输出
    bool search_element(K); // 查找元素
    bool delete_element(K); // 删除元素
    void dump_file();  // 存储到文件
    void load_file();   // 读取文件
    int size(); // 当前跳表有多少节点
    
private:
    // 辅助函数
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);
private:
    int _max_level;
    int _skip_list_level;
    Node<K, V>* _header;
    int _element_count;
    std::ofstream _file_writer;
    std::ifstream _file_reader;
};

// 创建新节点
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* n = new Node<K, V>(k, v, level);
    return n;
}





// 插入一个节点到跳表
// 返回0代表插入成功， 1代表失败
/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+
*/
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    // 因为插入元素，所以申请锁
    mtx.lock();

    // 1.先进行查找元素插入的位置
    // 先从跳表的头开始搜寻
    Node<K, V>* current = this->_header;
    // update记录插入位置的前一个节点是谁，每个节点都有可能有[1, _max_level+1]层指向
    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level+1));

    // 跳表从上至下，从前往后进行位置查找
    for(int i = _skip_list_level; i>=0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        // 每一层查找到的最后的节点都需要进行记录，方便后续更新
        update[i] = current;
    }

    // 2. 插入
    // 确定插入的位置，forward[0]代表该节点第0层的指向
    current = current->forward[0];
    // 先判断是否可插入
    // 不允许插入重复元素
    if(current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // 可插入
    if(current == NULL || current->get_key() != key) {
        // 随机的得到该节点最高可指向第几层
        int random_level = get_random_level();

        // 若可指向的层数高于当前已有层数
        // 那么从下至上进行更新，且更新已有层数
        if(random_level > _skip_list_level) {
            for(int i = _skip_list_level+1; i <= random_level; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // 生成需要插入的元素节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // 每一层都进行逐层的插入
        for(int i=0; i<=random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        // 更新已有节点数量
        _element_count ++;
    }

    // 释放锁
    mtx.unlock();
    return 0;
}


// 图像化跳表
template<typename K, typename V>
void SkipList<K, V>::display_list() {
    std::cout << "\n*****Skip List*****"<<"\n"; 
    for(int i=0; i <= _skip_list_level; i++) {
        Node<K, V>* node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while(node != NULL) {
            std::cout << node->get_key() << "(" << node->get_value() << ")->";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 构造函数
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {
    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
}

// 析构函数
template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    // 判断是否需要关闭文件
    if(_file_writer.is_open()) {
        _file_writer.close();
    }
    if(_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

// 随机生成层数，因为算法思想是二分的，其层数增长自然也是以50%的概率增长
template<typename K, typename V>
int SkipList<K, V>::get_random_level() {
    // k最小得为1层
    int k = 1;
    // rand()随机生成一个整数，让k以百分之五十的概率递增
    while(rand() % 2) {
        k++;
    }
    // k不可超过设置的阈值
    k = std::min(k, _max_level);

    return k;
}


// 从跳表中进行删除元素，类似于插入的逆操作
// true代表成功，false代表失败
template<typename K, typename V>
bool SkipList<K, V>::delete_element(K key) {
    // 1. 查找需要删除节点的位置
    mtx.lock();
    Node<K, V>* current = this->_header;
    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    for(int i = _skip_list_level; i>=0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    // 2. 判断是否进行删除
    if(current != NULL && current->get_key() == key) {
        // 层数从下至上进行删除，当到该节点最高层时选择退出
        for(int i = 0; i <= _skip_list_level; i++) {
            if(update[i]->forward[i] != current)
                break;
            update[i]->forward[i] = current->forward[i];
        }

        // 若已有层数是由该节点一柱擎天，那么进行更新
        while(_skip_list_level > 0 && _header->forward[_skip_list_level]==0) {
            _skip_list_level--;
        }
        std::cout << "Successfully deleted key "<< key << std::endl;
        _element_count --;
        mtx.unlock();
        return true;
    }


    mtx.unlock();
    return false;
}



/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/
// 查找元素
// 步骤原理，类似插入元素的前半段
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {
    std::cout << "search_element-----------------" << std::endl;
    Node<K, V>* current = _header;

    for(int i=_skip_list_level; i>=0; i--) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if(current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    
    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 存储到文件
// 格式：key:value 换行
template<typename K, typename V>
void SkipList<K, V>::dump_file() {
    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];

    while(node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }
    _file_writer.flush();
    _file_writer.close();
    return ;
}

// 从文件中读取跳表
template<typename K, typename V>
void SkipList<K, V>::load_file() {
    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();

    while(getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if(key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// 得到跳表的元素数量
template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}

// 类似反序列化
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

// 判断从文件中进行读取时，每行是否是有效的数据
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    if(str.empty()) {
        return false;
    }
    if(str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}
#endif