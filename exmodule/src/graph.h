#ifndef _GRAPH_H
#define _GRAPH_H

#include <cstddef>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <string>
#include <iostream>
#include <sstream>

#include <Python.h>

namespace cmaxflow {

template <typename FlowType> struct Edge;
template <typename FlowType> struct Node;


template <typename FlowType>
struct Edge {
  Node<FlowType>* src;
  Node<FlowType>* dst;
  FlowType flow;
  FlowType capacity;
  Edge<FlowType>* reversed;
}

template <typename FlowType>
struct Node {
  int name;
  size_t index;

  // -- For maxflow algorithm --
  FlowType excess;
  int height;

  size_t current_edge_idx;
  Node<FlowType>* next_in_bucket;
  Node<FlowType>* prev_in_bucket;
}

template <typename FlowType>
class Graph {
public:
  Graph();
  ~Graph();

  void Reset();

  typedef std::vector<Edge<Flow_Type>*> EdgeList;

  bool FromEdgeList(std::vector<std::pair<int, int>>* edge_list,
    std::vector<FlowType>* capacities, bool check_edge_redundancy);
  bool FromPyObject(PyObject* p, bool check_edge_redundancy);

  size_t GetNodeNumber();
  size_t GetEdgeNumber();

  Edge* AddEdge(Node* src, Node* dst, FlowType capacity);
  Node* AddNode(int name);

  std::string ToString();
  PyObject* ToPythonString();


private:
  size_t node_number_;
  size_t edge_number_;

  std::map<int, Node*> name_map_;
  std::vector<Node*> node_list_;
  std::vector<EdgeList> adjacency_list_;

}


// Implementation
Graph::Graph() {}

Graph::~Graph() {}

void Graph::Reset(){
  node_number_ = 0;
  edge_number_ = 0;
  index_map_.clear();
  node_list_.clear();
  adjacency_list_.clear();
}

template <typename FlowType>
size_t Graph::GetNodeNumber() {
  return node_number_;
}

template <typename FlowType>
size_t Graph::GetEdgeNumber() {
  return edge_number_;
}

// Add a new node with a given integer-valued name, and return its pointer.
// If the node already exists, this method returns the existing one.
template <typename FlowType>
Node<FlowType>* Graph::AddNode(int name) {
  if (name_map_.count(name) != 0) {
    return name_map_[name];
  }
  Node<FlowType> node;
  node.name = name;

  node.excess = 0;
  node.height = 0;
  node.current_edge_idx = 0;
  node.next_in_bucket = NULL;
  node.prev_in_bucket = NULL;
  node_list_.push_back(node);
  node.index = node_list_.size() - 1;
  name_map_[name] = &node_list_[node.index];

  node_number_ += 1;
  return &node_list_[node.index];
}

template <typename FlowType>
Edge<FlowType>* Graph::AddEdge(Node<FlowType>* src, Node<FlowType>* dst, FlowType capacity) {
  Edge edge;
  edge.src = src;
  edge.dst = dst;
  edge.flow = 0;
  edge.capacity = capacity;
  edge.reversed = NULL;
  size_t index = src->index;
  adjacency_list_[index].push_back(edge);
  edge_number_ += 1;
  return &adjacency_list_[index]
}

template <typename FlowType>
bool Graph::FromEdgeList(std::vector<std::pair<int, int>>* edge_list,
  std::vector<FlowType>* capacities, bool check_edge_redundancy) {
  size_t n = edge_list.size();
  if (capacities.size() != n) {
    std::cerr << "Warning: sizes of edge_list and capacities mismatch." << std::endl;
    return false;
  }
  for (size_t i = 0; i < n; i++) {
    int src = edge_list[i].first;
    int dst = edge_list[i].second;
    Node* src_node = AddNode(src);
    Node* dst_node = AddNode(dst);

    EdgeList* from_src = &adjacency_list_[src_node->index];
    EdgeList* from_dst = &adjacency_list_[dst_node->index];

    if (check_edge_redundancy) {
      // Add a new pair of an edge and a reversed edge with zero capacity.
      // If there is an existing pair, increase the capacity value.
      auto edge_it = std::find_if(from_src.begin(), from_src.end(),
        [](Edge<FlowType>* e){
          return e->src->index == src_node->index && e->dst->index == dst_node->index;
        });
      if (edge_it != from_src.end()) {
        *edge_it->capacity += capacities[i];
        //*edge_it->reverced->capacity += 0;
      }
      else {
        Edge<FlowType>* new_edge = AddEdge(src_node, dst_node, capacities[i]);
        Edge<FlowType>* new_edge_rev = AddEdge(dst_node, src_node, 0);
        new_edge->reversed = new_edge_rev;
        new_edge_rev->reversed = new_edge;
        *from_src.push_back(new_edge);
        *from_dst.push_back(new_edge_rev);
      }
    }
    else {
      // Add a new edge pair without redundancy check.
      Edge<FlowType>* new_edge = AddEdge(src_node, dst_node, capacities[i]);
      Edge<FlowType>* new_edge_rev = AddEdge(dst_node, src_node, 0);
      new_edge->reversed = new_edge_rev;
      new_edge_rev->reversed = new_edge;
      *from_src.push_back(new_edge);
      *from_dst.push_back(new_edge_rev);
    }
  }
  return true;
}

template <typename FlowType>
bool Graph::FromPyObject(PyObject* p, bool check_edge_redundancy) {
  std::vector<std::pair<int, int>> edge_list;
  std::vector<FlowType> capacities;
  if (!py_list_to_edge_list(p, &edge_list, &capacities)) {
    std::cerr << "Failed to convert a Python object to vectors." << std::endl;
    return false;
  }
  if (!FromEdgeList(&edge_list, &capacities, check_edge_redundancy)) {
    return false;
  }
  return true;
}

// Convert to a string object in the NetworkX Edge Lists format.
// See e.g. https://networkx.github.io/documentation/stable/reference/readwrite/edgelist.html
template <typename FlowType>
std::string Graph::ToString() {
  auto edge_to_str = [&](Edge<FlowType>* e) {
    // Convert an edge into a string like "src dst { 'capacity': cap, 'flow': flow }"
    std::ostringstream ss;
    ss << e->src->name << " " << e->dst->name << " ";
    ss << "{ 'capacity': " << e->capacity << ", 'flow': " << e->flow << "}";
    return ss.str();
  };

  std::ostringstream ss;

  for (auto edgelist_it = adjacency_list_.begin(); adjacency_list_.end(); edgelist_it++) {
    for (auto edge_it = *edgelist_it.begin(); *edgelist_it.end(); edge_it++) {
      ss << edge_to_str(*edge_it) << "\n";
    }
  }

  return ss.str();
}

template <typename FlowType>
PyObject* Graph::ToPythonString() {
  std::string str = ToString();
  return PyUnicode_FromString(str.data());
}


}

#endif
