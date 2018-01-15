import networkx as nx


def digraph_to_edge_list(g, capacity = 'capacity'):
    """
    Convert a NetworkX DiGraph object into an edge list that is acceptable to
    CythonGraph.from_py_object
    """
    if not nx.is_directed(g):
        raise ValueError("Input graph is not a DiGraph object")

    out = []
    for (src, dst, data) in g.edges.data():
        cap = data.get(capacity, 0.0)
        out.append((src, dst, {'capacity': cap}))

    return out


cdef extern from "src/graph.h" namespace "cmaxflow":
    cdef cppclass Graph:
        Graph()
        void Reset()
        bool FromPyObject(object edge_list)
        int GetNodeNumber()
        int GetEdgeNumber()
        str ToPythonString()


cdef class CythonGraph:
    cdef Graph* thisptr

    def __cinit__(self):
        self.thisptr = new Graph()

    def __dealloc__(self):
        del self.thisptr

    def from_py_object(self, object edge_list):
        self.thisptr.FromPyObject(edge_list)

    def get_node_number(self):
        return self.thisptr.GetNodeNumber()

    def get_edge_number(self):
        return self.thisptr.GetEdgeNumber()

    def __str__(self):
        return self.thisptr.ToPythonString()
