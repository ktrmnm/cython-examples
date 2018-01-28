#ifndef _MAXFLOW_H
#define _MAXFLOW_H

#include <Python.h>
#include <vector>
#include <algorithm>
#include <list>
#include <limits>
#include <deque>
#include <iostream>

#include "graph.h"
#include "utils.h"

//#define MAXFLOW_VERBOSE

namespace cmaxflow {

template <typename FlowType> class MaxflowGraph;

typedef MaxflowGraph<double> MaxflowGraphDouble;
typedef MaxflowGraph<int> MaxflowGraphInt;

template <typename FlowType>
class MaxflowGraph{
public:
  MaxflowGraph();
  MaxflowGraph(size_t max_node_num);
  ~MaxflowGraph();

  //bool FromEdgeList(std::vector<std::pair<int, int>> edge_list,
  //  std::vector<FlowType> capacities, bool check_edge_redundancy);
  bool FromPyObject(PyObject* p, bool check_edge_redundancy);
  //bool SetSourceSink(PyObject* s, PyObject* t);
  bool SetSourceSink(int s, int t);
  //bool SetTol(PyObject* tol);

  FlowType MaxPreFlow(unsigned int global_relabel_frequency, FlowType tol);
  void MinCut();

  //PyObject* ToPythonResidualGraph();
  PyObject* ToPythonMinCut();

private:
  Graph<FlowType> graph_;

  size_t source_index_;
  size_t sink_index_;
  bool IsInnerNode(Node<FlowType>* node) {
    return node->index != source_index_ && node->index != sink_index_;
  }

  bool done_maxflow_;
  FlowType flow_value_;
  bool done_mincut_;
  std::vector<bool> reacheable_from_sink_;

  FlowType tol_;
  bool IsClose(FlowType a, FlowType b) {
    return isclose<FlowType>(a, b, tol_);
  }

  std::vector<std::list<Node<FlowType>*>> active_nodes_;
  std::vector<std::list<Node<FlowType>*>> inactive_nodes_;
  int max_height_;

  void InitNodes();
  void InitFlows();
  void InitBuckets();
  void Discharge(Node<FlowType>* node);
  void Push(Edge<FlowType>* edge, FlowType amount);
  bool Relabel(Node<FlowType>* node);
  void GapHeuristic(int height);

  unsigned int global_relabel_counter_;
  unsigned int global_relabel_threshold_;
  void GlobalRelabeling();

};


// Implementation

template <typename FlowType>
MaxflowGraph<FlowType>::MaxflowGraph() {
  graph_ = Graph<FlowType>();
  done_maxflow_ = false;
  done_mincut_ = false;
}

template <typename FlowType>
MaxflowGraph<FlowType>::MaxflowGraph(size_t max_node_num) {
  graph_ = Graph<FlowType>(max_node_num);
  done_maxflow_ = false;
  done_mincut_ = false;
}

template <typename FlowType>
MaxflowGraph<FlowType>::~MaxflowGraph() {}

template <typename FlowType>
bool MaxflowGraph<FlowType>::FromPyObject(PyObject* p, bool check_edge_redundancy){
  if (!graph_.FromPyObject(p, check_edge_redundancy)) {
    return false;
  }
  //source_index_ = graph_.GetNodeByName(s_name)->index;
  //sink_index_ = graph_.GetNodeByName(t_name)->index;
  return true;
}

/*
template <typename FlowType>
bool MaxflowGraph<FlowType>::SetSourceSink(PyObject* s, PyObject* t) {
  int s_name = py_int_to_int(s);
  int t_name = py_int_to_int(t);
  if (s_name == -1 || t_name == -1) {
    std::cerr << "Warning: failed to convert a Python object to int." << std::endl;
    return false;
  }
  Node<FlowType>* source = graph_.GetNodeByName(s_name);
  Node<FlowType>* sink = graph_.GetNodeByName(t_name);
  if (source == nullptr || sink == nullptr) {
    std::cerr << "Warning: source or sink node are not found in graph." << std::endl;
    return false;
  }
  else {
    source_index_ = source->index;
    sink_index_ = sink->index;
    return true;
  }
}
*/
template <typename FlowType>
bool MaxflowGraph<FlowType>::SetSourceSink(int s, int t) {
  Node<FlowType>* source = graph_.GetNodeByName(s);
  Node<FlowType>* sink = graph_.GetNodeByName(t);
  if (source == nullptr || sink == nullptr) {
    std::cerr << "Warning: source or sink node are not found in graph." << std::endl;
    return false;
  }
  else {
    source_index_ = source->index;
    sink_index_ = sink->index;
    return true;
  }
}

// Initialize some node information:
template <typename FlowType>
void MaxflowGraph<FlowType>::InitNodes() {
  size_t n = graph_.GetNodeNumber();
  for (size_t i = 0; i < n; i++) {
    Node<FlowType>* node = graph_.GetNode(i);
    int height = 1;
    if (i == source_index_) {
      height = (int) n;
    }
    else if (i == sink_index_) {
      height = 0;
    }
    node->height = height;
    node->excess = 0;
    node->current_edge_idx = 0;
    if (height < (int) n) {
      inactive_nodes_[height].push_back(node);
    }
  }
  //graph_.GetNode(sink_index_)->height = 0;
  //graph_.GetNode(source_index_)->height = (int) n;
  max_height_ = 0;
}

template <typename FlowType>
void MaxflowGraph<FlowType>::InitBuckets() {
  size_t n = graph_.GetNodeNumber();
  active_nodes_.clear();
  inactive_nodes_.clear();
  active_nodes_.reserve(n);
  inactive_nodes_.reserve(n);
  for (size_t i = 0; i < n; i++) {
    active_nodes_.push_back(std::list<Node<FlowType>*>());
    inactive_nodes_.push_back(std::list<Node<FlowType>*>());
  }
}

// Initialize preflows by zero
template <typename FlowType>
void MaxflowGraph<FlowType>::InitFlows() {
  size_t n = graph_.GetNodeNumber();
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < graph_.GetOutEdgeNumber(i); j++) {
      Edge<FlowType>* edge = graph_.GetEdge(i, j);
      edge->flow = 0;
    }
  }
}

template <typename FlowType>
FlowType MaxflowGraph<FlowType>::MaxPreFlow(unsigned int global_relabel_frequency, FlowType tol) {
  tol_ = tol;
  global_relabel_counter_ = 0;
  if (global_relabel_frequency == 0) {
    global_relabel_threshold_ = std::numeric_limits<FlowType>::max();
  }
  else {
    unsigned int nm = (unsigned int) (graph_.GetNodeNumber() + graph_.GetEdgeNumber());
    global_relabel_threshold_ = nm / global_relabel_frequency;
  }

  // Init nodes and edges
  InitBuckets();
  InitNodes();
  InitFlows();


  // Push all edges from source
  for (size_t i = 0; i < graph_.GetOutEdgeNumber(source_index_); i++) {
    Edge<FlowType>* edge = graph_.GetEdge(source_index_, i);
    FlowType res = edge->capacity - edge->flow;
    if (res > 0 && !IsClose(res, 0)) {
      Push(edge, res);
    }
  }

  // Main loop
  while (true) {
    // Global relabeling
    if (global_relabel_counter_ > global_relabel_threshold_) {
      GlobalRelabeling();
      global_relabel_counter_ = 0;
    }

    // Pop an active node to discharge
    // If there is no active node, then stop.
    while (max_height_ >= 0 && active_nodes_[max_height_].empty()) {
      max_height_ -= 1;
    }
    if (max_height_ < 0) {
      #ifdef MAXFLOW_VERBOSE
      std::cout << "Done!" << std::endl;
      #endif
      break;
    }
    Node<FlowType>* node = active_nodes_[max_height_].back();
    active_nodes_[max_height_].pop_back();

    // Discharge node
    Discharge(node);
  }
  done_maxflow_ = true;
  flow_value_ = graph_.GetNode(sink_index_)->excess;
  return flow_value_;
}

// Increase flow value of the given edge
template <typename FlowType>
void MaxflowGraph<FlowType>::Push(Edge<FlowType>* edge, FlowType amount) {
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Pushing edge (" << edge->src->name << ", " << edge->dst->name << ")"
  << " (amount: "<< amount
  << ", residual cap:" << edge->capacity - edge->flow << ")" << std::endl;
  //std::cout << "[amount = " << amount << ", res = " << edge->capacity - edge->flow << "]" << std::endl;
  #endif
  edge->flow += amount;
  graph_.GetReversedEdge(edge)->flow -= amount;

  Node<FlowType>* src = edge->src;
  src->excess -= amount;

  Node<FlowType>* dst = edge->dst;
  if (IsClose(dst->excess, 0) && IsInnerNode(dst)) {
    // inactive_nodes_から削除してactive_nodes_に追加したいが，
    // list内での現在の位置がわからないのでで O(1) ではできない．
    // 今回の実装では諦めてlist内をサーチしてよいことにする
    inactive_nodes_[dst->height].remove(dst);
    active_nodes_[dst->height].push_back(dst);
    max_height_ = std::max(dst->height, max_height_);
    #ifdef MAXFLOW_VERBOSE
    std::cout << "node " << dst->name << " is activated "
    "(height = " << dst->height << ")" << std::endl;
    #endif
  }
  dst->excess += amount;

}

// Relabel node
// The return value is used to decide termination of the discharge operation.
// If a gap found at node->height, then conduct the gap heuristics
// and return false.
// Otherwise relabel node to make at least one admissible edge.
// If the new height is less than n = |V|, then return true.
// It is known that if the node is not reachable from the sink node height >= n.
// Therefore, in order to find the maximum preflow, we can stop the discharge
// operation if height >= n.
template <typename FlowType>
bool MaxflowGraph<FlowType>::Relabel(Node<FlowType>* node) {
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Relabeling node " << node->name << " (height: "
  << node->height << ")" << std::endl;
  #endif
  int n = (int) graph_.GetNodeNumber();
  global_relabel_counter_ += n;

  int old_height = node->height;
  if (active_nodes_[old_height].size() == 0 && inactive_nodes_[old_height].size() == 0){
    GapHeuristic(old_height);
    node->height = n;
    return false;
  }

  //int min_height = std::numeric_limits<int>::max();
  int min_height = 2 * n;
  size_t min_edge_index = 0;
  for (size_t i = 0; i < graph_.GetOutEdgeNumber(node->index); i++) {
    Edge<FlowType>* edge = graph_.GetEdge(node->index, i);
    int current_height = edge->dst->height;
    FlowType res = edge->capacity - edge->flow;
    //std::cout << "edge (" << node->name << ", " << edge->dst->name << ")"
    //<< "flow: " << edge->flow << " cap: " << edge -> capacity << std::endl;
    if (res > 0 && !IsClose(res, 0) && min_height > current_height) {
      min_height = current_height;
      min_edge_index = i;
    }
  }
  node->current_edge_idx = min_edge_index;
  node->height = min_height + 1;
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Height of node " << node->name << " is changed: " << old_height
  << " -> " << node->height << std::endl;
  #endif
  return node->height < n;
}

template <typename FlowType>
void MaxflowGraph<FlowType>::Discharge(Node<FlowType>* node) {
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Discharging node " << node->name << " (height: " << node->height
  << ", excess: " << node->excess << ")" << std::endl;
  #endif
  while (true) {
    Edge<FlowType>* current_edge = graph_.GetEdge(node->index, node->current_edge_idx);
    FlowType res = current_edge->capacity - current_edge->flow;
    if (res > 0 && !IsClose(res, 0)) {
      Node<FlowType>* dst = current_edge->dst;
      // tbc: current edge is admissible if dst->height + 1 == node->height
      if (dst->height <  node->height) {
        FlowType update = std::min(node->excess, res);
        Push(current_edge, update);
        if (IsClose(node->excess, 0)) {
          break;
        }
      }
    }
    // If the current edge is the last edge in the adjacency list, then
    // try to relabel node and make an admissible edge.
    if (node->current_edge_idx == graph_.GetOutEdgeNumber(node->index) - 1) {
      if (!Relabel(node)) {
        break;
      }
    }
    else {
      node->current_edge_idx += 1;
    }
  }
  if (node->height < (int) graph_.GetNodeNumber()) {
    if (node->excess > 0 && !IsClose(node->excess, 0)) {
      active_nodes_[node->height].push_back(node);
      max_height_ = std::max(node->height, max_height_);
      #ifdef MAXFLOW_VERBOSE
      std::cout << "node " << node->name << " is still active " <<
      "(max_height = " << max_height_ << ")" << std::endl;
      #endif
    }
    else {
      inactive_nodes_[node->height].push_back(node);

      #ifdef MAXFLOW_VERBOSE
      std::cout << "node " << node->name << " is deactivated "
      "(max_height = " << max_height_ << ")" << std::endl;
      #endif
    }
  }
}

template <typename FlowType>
void MaxflowGraph<FlowType>::GapHeuristic(int height) {
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Gap relabeling at height = " << height << std::endl;
  #endif
  int n = (int) graph_.GetNodeNumber();

  for (int h = height; h <= max_height_; h++) {
    for (auto node_it = active_nodes_[h].begin();
        node_it != active_nodes_[h].end(); node_it++) {
      (*node_it)->height = n;
    }
    active_nodes_[h].clear();

    for (auto node_it = inactive_nodes_[h].begin();
        node_it != inactive_nodes_[h].end(); node_it++) {
      (*node_it)->height = n;
    }
    inactive_nodes_[h].clear();
  }
  max_height_ = height - 1;
}

template <typename FlowType>
void MaxflowGraph<FlowType>::GlobalRelabeling() {
  #ifdef MAXFLOW_VERBOSE
  std::cout << "Global update" << std::endl;
  #endif
  std::deque<Node<FlowType>*> Q;
  std::vector<bool> visited(graph_.GetNodeNumber(), false);

  Node<FlowType>* sink = graph_.GetNode(sink_index_);
  Q.push_back(sink);
  visited[sink->index] = true;
  InitBuckets();

  while (!Q.empty()) { // start bfs
    Node<FlowType>* node = Q.front();
    Q.pop_front();
    //std::cout << "node = " << node->name << std::endl;
    int next_height = node->height + 1;

    for (size_t i = 0; i < graph_.GetOutEdgeNumber(node->index); i++) {
      Edge<FlowType>* edge = graph_.GetEdge(node->index, i);
      Edge<FlowType>* edge_rev = graph_.GetReversedEdge(edge);
      FlowType res_rev = edge_rev->capacity - edge_rev->flow;
      if (res_rev > 0 && !IsClose(res_rev, 0)) {
        Node<FlowType>* next_node = edge->dst;
        if (!visited[next_node->index]) {
          visited[next_node->index] = true;
          next_node->height = next_height;
          if (next_node->excess > 0 && !IsClose(next_node->excess, 0)) {
            active_nodes_[next_height].push_back(next_node);
            max_height_ = std::max(next_height, max_height_);
          }
          else {
            inactive_nodes_[next_height].push_back(next_node);
          }
          Q.push_back(next_node);
        }
      }
    }
  } // end bfs
  int n = (int) graph_.GetNodeNumber();
  for (size_t i = 0; i < graph_.GetNodeNumber(); i++) {
    Node<FlowType>* node = graph_.GetNode(i);
    if (IsInnerNode(node)) {
      node->current_edge_idx = 0;
      if (!visited[i]) {
        node->height = n;
      }
    }
  }
}

template <typename FlowType>
void MaxflowGraph<FlowType>::MinCut() {
  if (!done_maxflow_) {
    std::cerr << "Warning: MinCut must be called after MaxPreFlow." << std::endl;
  }
  else {
    #ifdef MAXFLOW_VERBOSE
    std::cout << "Calculate mincut" << std::endl;
    #endif
    size_t n = graph_.GetNodeNumber();
    reacheable_from_sink_.reserve(n);
    reacheable_from_sink_.assign(n, false);

    std::deque<Node<FlowType>*> Q;
    std::vector<bool> visited(n, false);

    Node<FlowType>* sink = graph_.GetNode(sink_index_);
    Q.push_back(sink);
    visited[sink_index_] = true;
    reacheable_from_sink_[sink_index_] = true;

    while (!Q.empty()) { //bfs
      Node<FlowType>* node = Q.front();
      Q.pop_front();
      for (size_t i = 0; i < graph_.GetOutEdgeNumber(node->index); i++) {
        Edge<FlowType>* edge = graph_.GetEdge(node->index, i);
        Edge<FlowType>* edge_rev = graph_.GetReversedEdge(edge);
        FlowType res_rev = edge_rev->capacity - edge_rev->flow;
        if (res_rev > 0 && !IsClose(res_rev, 0)) {
          Node<FlowType>* next_node = edge->dst;
          if (!visited[next_node->index]) {
            visited[next_node->index] = true;
            reacheable_from_sink_[next_node->index] = true;
            Q.push_back(next_node);
          }
        }
      }
    }//bfs end

    done_mincut_ = true;
  }
}

template <typename FlowType>
PyObject* MaxflowGraph<FlowType>::ToPythonMinCut(){
  if (!done_mincut_) {
    std::cerr << "Warning: ToPythonMinCut must be called after MaxCut." << std::endl;
    return NULL;
  }
  else {
    #ifdef MAXFLOW_VERBOSE
    std::cout << "Convert to Python object" << std::endl;
    #endif
    size_t n = graph_.GetNodeNumber();
    PyObject* cut = PySet_New(NULL);
    PyObject* cut_c = PySet_New(NULL);
    for (size_t i = 0; i < n; i++) {
      int name = graph_.GetNode(i)->name;
      if (reacheable_from_sink_[i]) {
        PySet_Add(cut_c, PyLong_FromLong((long) name));
      }
      else {
        PySet_Add(cut, PyLong_FromLong((long) name));
      }
    }
    PyObject* partition = PyTuple_Pack(2, cut, cut_c);
    PyObject* flow = PyFloat_FromDouble((double) flow_value_);
    PyObject* ret = PyTuple_Pack(2, flow, partition);

    Py_XDECREF(cut);
    Py_XDECREF(cut_c);
    Py_XDECREF(partition);
    Py_XDECREF(flow);
    return ret;
  }
}

}

#endif
