#ifndef _UTILS_H
#define _UTILS_H

//#define VERBOSE

#ifdef VERBOSE
#include <iostream>
#endif

#include <cmath>
#include <utility>
#include <Python.h>

template <typename T>
bool isclose(T a, T b, T abs_tol) {
  return abs(a - b) < abs_tol;
}

/*
std::pair<int, int> py_tuple_to_int_pair(PyObject* py_tuple) {
  auto set_value_error = [&]{
    PyErr_SetObject(PyExc_ValueError,
      PyUnicode_FromFormat(
        "Invalid format of Python object (expected: tuple(int, int)): %S",PyObject_Str(py_tuple)
      )
    );
  };

  if (!PyTuple_Check(py_tuple)) {
    set_value_error();
    return nullptr;
  }
  Py_ssize_t size = PyTuple_Size(py_tuple);
  if (size < 2) {
    set_value_error();
    return nullptr;
  }

  if (size > 2) {
    std::cerr << "Warning: Length of Python tuple is larger than 2." << std::endl;
  }

  int first = (int) PyLong_AsLong(PyTuple_GetItem(py_tuple, 0));
  int second = (int) PyLong_AsLong(PyTuple_GetItem(py_tuple, 1));
  if (PyErr_Occurred()) {
    set_value_error();
    return nullptr;
  }
  return std::make_pair(first, second);
}

std::vector<PyObject*> py_list_to_py_object_vector(PyObject* py_list) {
  auto set_value_error = [&]{
    PyErr_SetObject(PyExc_ValueError,
      PyUnicode_FromFormat(
        "Invalid format of Python object (expected: list): %S", PyObject_Str(py_list)
      )
    );
  }

  if (PyList_Check(py_list)) {
    set_value_error();
    return nullptr;
  }
  std::vector<PyObject*> out;
  for (Py_ssize_t i = 0; i < PyList_Size(py_list); i++) {
    out.push_back(PyList_GetItem(py_list, i));
  }
  return out;
}
*/

// Convert a Python edge list into an edge list (vector<pair<int, int>>)
// and a capacity vector (vector<double>). Both return values are set in-place.
// The input Python object must be a list of tuples, and each tuple represent an
// edge (u, v, {'capacity': capacity}).
bool py_list_to_edge_list(PyObject* py_list,
  std::vector<std::pair<int, int>>* edge_list, std::vector<double>* capacities) {

  auto set_value_error = [&](PyObject* p, char* expected){
    PyErr_SetObject(PyExc_ValueError,
      PyUnicode_FromFormat(
        "Invalid format of Python object (expected: %s): %S", expected, PyObject_Str(p)
      )
    );
  };

  if (!PyList_Check(py_list)){
    set_value_error(py_list, (char *) "list");
    return false;
  }

  Py_ssize_t size = PyList_Size(py_list);
  edge_list->clear();
  edge_list->reserve((size_t) size);
  capacities->clear();
  capacities->reserve((size_t) size);

  for (Py_ssize_t i = 0; i < size; i++) {
    PyObject* py_edge = PyList_GetItem(py_list, i);

    if (!PyTuple_Check(py_edge) || PyTuple_Size(py_edge) < 3) {
      set_value_error(py_edge, (char *) "tuple(u, v, {'capacity': c})");
      return false;
    }
    PyObject* py_src = PyTuple_GetItem(py_edge, 0);
    PyObject* py_dst = PyTuple_GetItem(py_edge, 1);
    PyObject* py_dict = PyTuple_GetItem(py_edge, 2);
    if (!PyLong_Check(py_src) || !PyLong_Check(py_dst) || !PyDict_Check(py_dict)) {
      set_value_error(py_edge, (char *) "tuple(u, v, {'capacity': c})");
      return false;
    }
    PyObject* py_capacity = PyDict_GetItemString(py_dict, "capacity");
    if (py_capacity == NULL || !PyFloat_Check(py_capacity)) {
      set_value_error(py_edge, (char *) "tuple(u, v, {'capacity': c})");
      return false;
    }
    int src = (int) PyLong_AsLong(py_src);
    int dst = (int) PyLong_AsLong(py_dst);
    double cap = (double) PyFloat_AsDouble(py_capacity);
    #ifdef VERBOSE
    std::cout << "added an edge (src: " << src << ", dst: " << dst << ", cap:"
    << cap << ")" << std::endl;
    #endif
    edge_list->push_back(std::make_pair(src, dst));
    capacities->push_back(cap);
  }
  return true;
}


#endif
