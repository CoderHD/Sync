#pragma once
#include "Types.h"

#if defined(EMBEDDED)
uint hash(u32 ind, const uint N) { return ind % N; };

constexpr uint N = 10;
template <typename T>
class HashMap {
 private:
  struct Node {
    T val;
    Node *next;
  };
  Node  nodes[N];
  uint currNode;
  Node *table[N];
  void nextNode() { currNode = (currNode + 1) % N; }
 public:
  HashMap() { currNode = 0; }
  
  bool insert(u32 id, const T &val) {
    if(currNode == N) {
      return false;
    }
    else {
      uint ind = hash(id, N);
      Node *node = table[ind];
      if(node) {
	while(node) {
	  if(node->next) node = node->next;
	  else {
	    node->next = &nodes[currNode];
	    node->next->val = val;
	  }
	}
      } else {
	table[ind] = &nodes[currNode];
	table[ind]->val = val;
      }
      nextNode();
      return true;
    }
  };

  T *find(u32 id) {
    Node *node = table[hash(id, N)];
    if(node) {
      while(node) {
	if(node->val == val) return &val;
	else node = node->next;
      }
    }
    return null;
  };
};
#elif defined(LIVE)
#include <unordered_map>
template <typename T>
class HashMap {
 private:
  std::unordered_map<int, T> map;
 public:
  HashMap() {};
  
  bool insert(u32 id, const T& val) { 
    map.emplace(id, val); 
    return true; 
  };
  
  T *find(u32 id) { 
    auto found = map.find(id); 
    return (found == map.end()) ? null : &found->second; 
  };
};
#endif
