#ifndef _GRAPH_H
#define _GRAPH_H

//#define VERBOSE

#include <cstddef>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <string>
#include <iostream>
#include <sstream>

#include <Python.h>

#include "utils.h"

namespace cmaxflow {

template <typename FlowType> struct Edge;
template <typename FlowType> struct Node;
template <typename FlowType> class Graph;

typedef Graph<double> GraphDouble;
typedef Graph<int> GraphInt;

template <typename FlowType>
struct Edge {
  Node<FlowType>* src;
  Node<FlowType>* dst;
  FlowType flow;
  FlowType capacity;
  //Edge<FlowType> reversed;
  size_t reversed;
};

template <typename FlowType>
struct Node {
  int name;
  size_t index;

  // -- For maxflow algorithm --
  FlowType excess;
  int height;

  size_t current_edge_idx;
  //Node<FlowType>* next_in_bucket;
  //Node<FlowType>* prev_in_bucket;
  size_t index_in_bucket;
};

template <typename FlowType>
class Graph {
public:
  Graph();
  Graph(size_t max_node_num);
  ~Graph();

  void Reset();

  typedef std::vector<Edge<FlowType>> EdgeList;

  bool FromEdgeList(std::vector<std::pair<int, int>> edge_list,
    std::vector<FlowType> capacities, bool check_edge_redundancy);
  bool FromPyObject(PyObject* p, bool check_edge_redundancy);

  size_t GetNodeNumber();
  size_t GetEdgeNumber();
  size_t GetOutEdgeNumber(size_t src_index);

  //Edge<FlowType>* AddEdge(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity);
  size_t AddEdge(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity);
  //void AddEdgePair(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity);
  Node<FlowType>* AddNode(int name);
  //size_t AddNode(int name);

  Node<FlowType>* GetNode(size_t index);
  Node<FlowType>* GetNodeByName(int name);
  Edge<FlowType>* GetEdge(size_t src_index, size_t edge_index);
  Edge<FlowType>* GetReversedEdge(Edge<FlowType>* edge);

  std::string ToString();
  PyObject* ToPythonString();


private:
  size_t max_node_num_;
  size_t node_number_;
  size_t edge_number_;

  //std::map<int, Node<FlowType>*> name_map_;
  std::map<int, size_t> name_map_;
  //std::vector<int> name_list_;
  std::vector<Node<FlowType>> node_list_;
  std::vector<EdgeList> adjacency_list_;

};


// Implementation
template <typename FlowType>
Graph<FlowType>::Graph() {
  max_node_num_ = 0;
  //Reset();
}

template <typename FlowType>
Graph<FlowType>::Graph(size_t max_node_num) {
  max_node_num_ = max_node_num;
  //Reset();
}

template <typename FlowType>
Graph<FlowType>::~Graph() {}

template <typename FlowType>
void Graph<FlowType>::Reset(){
  node_number_ = 0;
  edge_number_ = 0;
  name_map_.clear();
  //name_list_.clear();
  node_list_.clear();
  adjacency_list_.clear();
  if (max_node_num_ > 0) {
    node_list_.reserve(max_node_num_);
    adjacency_list_.reserve(max_node_num_);
  }
}

template <typename FlowType>
size_t Graph<FlowType>::GetNodeNumber() {
  return node_number_;
}

template <typename FlowType>
size_t Graph<FlowType>::GetEdgeNumber() {
  return edge_number_;
}

template <typename FlowType>
size_t Graph<FlowType>::GetOutEdgeNumber(size_t src_index) {
  return adjacency_list_[src_index].size();
}

template <typename FlowType>
Node<FlowType>* Graph<FlowType>::GetNode(size_t index) {
  return &node_list_[index];
}

template <typename FlowType>
Node<FlowType>* Graph<FlowType>::GetNodeByName(int name) {
  if (name_map_.count(name) != 0) {
    return &node_list_[name_map_[name]];
  }
  else {
    return nullptr;
  }
}

template <typename FlowType>
Edge<FlowType>* Graph<FlowType>::GetEdge(size_t src_index, size_t edge_index) {
  return &adjacency_list_[src_index][edge_index];
}

template <typename FlowType>
Edge<FlowType>* Graph<FlowType>::GetReversedEdge(Edge<FlowType>* edge) {
  return &adjacency_list_[edge->dst->index][edge->reversed];
}

// Add a new node with a given integer-valued name, and return its pointer.
// If the node already exists, this method returns the existing one.
template <typename FlowType>
Node<FlowType>* Graph<FlowType>::AddNode(int name) {
  if (name_map_.count(name) != 0) {
#ifdef VERBOSE
  std::cout << "node name " << name << " is already added. Returned node #"
  << node_list_[name_map_[name]].index << std::endl;
#endif
    return &node_list_[name_map_[name]];
    //return name_map_[name];
  }
  else {
    Node<FlowType> node;
    node.name = name;

    node.excess = 0;
    node.height = 0;
    node.current_edge_idx = 0;
    //node->next_in_bucket = nullptr;
    //node->prev_in_bucket = nullptr;
    node.index_in_bucket = 0;
    node.index = node_list_.size();
    node_list_.push_back(node);

    name_map_[name] = node.index;
    adjacency_list_.push_back(std::vector<Edge<FlowType>>());
  #ifdef VERBOSE
    std::cout << "added node #" << name << " to graph (index = " << node.index << ")" << std::endl;
  #endif
    node_number_ += 1;
    return &node_list_.back();
  }
}

template <typename FlowType>
size_t Graph<FlowType>::AddEdge(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity) {
  //Edge<FlowType>* edge = new Edge<FlowType>();
  Edge<FlowType> edge;
  edge.src = src;
  edge.dst = dst;
  edge.flow = 0;
  edge.capacity = capacity;
  edge.reversed = 0;
  size_t index = src->index;
  adjacency_list_[index].push_back(edge);
  edge_number_ += 1;
#ifdef VERBOSE
  std::cout << "added edge (" << src.name << ", " << dst.name << ") to graph" << std::endl;
#endif
  return adjacency_list_[index].size() - 1;
}

/*
template <typename FlowType>
void Graph<FlowType>::AddEdgePair(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity) {
  Edge<FlowType> edge;
  Edge<FlowType> edge_rev;
  edge.src = src;
  edge.dst = dst;
  edge.flow = 0;
  edge.capacity = capacity;
  edge_rev.src = dst;
  edge_rev.dst = src;
  edge_rev.flow = 0;
  edge_rev.capacity = 0;
  adjacency_list_[src->index].push_back(edge);
  adjacency_list_[dst->index].push_back(edge);
  (adjacency_list_[src->index].back()).reversed = &(adjacency_list_[dst->index].back());
  (adjacency_list_[dst->index].back()).reversed = &(adjacency_list_[src->index].back());
  edge_number_ += 2;
}
*/

template <typename FlowType>
bool Graph<FlowType>::FromEdgeList(std::vector<std::pair<int, int>> edge_list,
  std::vector<FlowType> capacities, bool check_edge_redundancy) {
  Reset();
  size_t n = edge_list.size();
  if (capacities.size() != n) {
    std::cerr << "Warning: sizes of edge_list and capacities mismatch." << std::endl;
    return false;
  }
#ifdef VERBOSE
  std::cout << "#edges = " << n << std::endl;
  std::cout << "check redundancy: " << check_edge_redundancy << std::endl;
#endif
  for (size_t i = 0; i < n; i++) {
    int src = edge_list[i].first;
    int dst = edge_list[i].second;
    Node<FlowType>* src_node = AddNode(src);
    //size_t src_node = AddNode(src);
    Node<FlowType>* dst_node = AddNode(dst);
    //size_t dst_node = AddNode(dst);

    EdgeList from_src = adjacency_list_[src_node->index];
    //EdgeList from_src = adjacency_list_[node_list_[src_node].index];
    EdgeList from_dst = adjacency_list_[dst_node->index];
    //EdgeList from_dst = adjacency_list_[node_list_[dst_node].index];

    if (check_edge_redundancy) {
      // Add a new pair of an edge and a reversed edge with zero capacity.
      // If there is an existing pair, increase the capacity value.
      auto edge_it = std::find_if(from_src.begin(), from_src.end(),
        [&](Edge<FlowType> e){
          return e.src->index == src_node->index && e.dst->index == dst_node->index;
        });
      if (edge_it != from_src.end()) {
        (*edge_it).capacity += capacities[i];
        //*edge_it->reverced->capacity += 0;
      }
      else {
        size_t new_edge = AddEdge(src_node, dst_node, capacities[i]);
        size_t new_edge_rev = AddEdge(dst_node, src_node, 0);
        GetEdge(src_node->index, new_edge)->reversed = new_edge_rev;
        GetEdge(dst_node->index, new_edge_rev)->reversed = new_edge;
      }
    }
    else {
      // Add a new edge pair without redundancy check.
      size_t new_edge = AddEdge(src_node, dst_node, capacities[i]);
      size_t new_edge_rev = AddEdge(dst_node, src_node, 0);
      GetEdge(src_node->index, new_edge)->reversed = new_edge_rev;
      GetEdge(dst_node->index, new_edge_rev)->reversed = new_edge;
    }
  }
  return true;
}

template <typename FlowType>
bool Graph<FlowType>::FromPyObject(PyObject* p, bool check_edge_redundancy) {
  std::vector<std::pair<int, int>> edge_list;
  std::vector<FlowType> capacities;
  if (!py_list_to_edge_list(p, &edge_list, &capacities)) {
    std::cerr << "Failed to convert a Python object to vectors." << std::endl;
    return false;
  }
  if (!FromEdgeList(edge_list, capacities, check_edge_redundancy)) {
    return false;
  }
  return true;
}

// Convert to a string object in the NetworkX Edge Lists format.
// See e.g. https://networkx.github.io/documentation/stable/reference/readwrite/edgelist.html
template <typename FlowType>
std::string Graph<FlowType>::ToString() {
  auto edge_to_str = [&](Edge<FlowType> e) {
    // Convert an edge into a string like "src dst { 'capacity': cap, 'flow': flow }"
    std::ostringstream ss;
    ss << e.src->name << " " << e.dst->name << " ";
    ss << "{ 'capacity': " << e.capacity << ", 'flow': " << e.flow << "}";
    return ss.str();
  };

  std::ostringstream ss;

  for (auto edgelist_it = adjacency_list_.begin(); edgelist_it != adjacency_list_.end(); edgelist_it++) {
    for (auto edge_it = edgelist_it->begin(); edge_it != edgelist_it->end(); edge_it++) {
      ss << edge_to_str(*edge_it) << "\n";
    }
  }

  return ss.str();
}

template <typename FlowType>
PyObject* Graph<FlowType>::ToPythonString() {
  std::string str = ToString();
  return PyUnicode_FromString(str.data());
}


}

#endif
