#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

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
#endif