#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>
#include <algorithm>

#define STORE_FILE "store/dumpFile"

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

    int get_random_level();
    Node<K, V>* create_node(K, V, int);
    int insert_element(K, V);
    void display_list();
    bool search_element(K);
    void delete_element(K);
    void dump_file();
    void load_file();
    int size();
    
private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);
private:
    int _max_level;

    int _skip_list_level;

    Node<K, V>* _header;

    std::ofstream _file_writer;
    std::ifstream _file_reader;

    int _element_count;
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
    mtx.lock();
    Node<K, V>* current = this->_header;

    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level+1));

    for(int i = _skip_list_level; i>=0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    if(current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    if(current == NULL || current->get_key() != key) {
        int random_level = get_random_level();

        if(random_level > _skip_list_level) {
            for(int i = _skip_list_level+1; i <= random_level; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        Node<K, V>* inserted_node = create_node(key, value, random_level);

        for(int i=0; i<=random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;
    }
    mtx.unlock();
    return 0;
}


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

template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    if(_file_writer.is_open()) {
        _file_writer.close();
    }
    if(_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

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
#endif