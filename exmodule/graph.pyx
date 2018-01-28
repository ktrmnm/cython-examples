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
    cdef cppclass GraphDouble:
        GraphDouble()
        GraphDouble(int max_node_num)
        void Reset()
        int FromPyObject(object edge_list, int check_edge_redundancy)
        int GetNodeNumber()
        int GetEdgeNumber()
        str ToPythonString()


cdef class CythonGraph:
    cdef GraphDouble* thisptr

    def __cinit__(self, int max_node_num = 128):
        self.thisptr = new GraphDouble(max_node_num)

    def __dealloc__(self):
        del self.thisptr

    def from_py_object(self, object edge_list):
        self.thisptr.FromPyObject(edge_list, 0)

    def get_node_number(self):
        return self.thisptr.GetNodeNumber()

    def get_edge_number(self):
        return self.thisptr.GetEdgeNumber()

    def __str__(self):
        return self.thisptr.ToPythonString()


cdef extern from "src/maxflow.h" namespace "cmaxflow":
    cdef cppclass MaxflowGraphDouble:
        MaxflowGraphDouble()
        MaxflowGraphDouble(int max_node_num)

        int FromPyObject(object edge_list, int check_edge_redundancy)
        int SetSourceSink(int s, int t)
        float MaxPreFlow(int global_relabel_frequency, float tol)
        void MinCut()
        object ToPythonMinCut()


cdef class CythonMaxflowGraph:
    cdef MaxflowGraphDouble* thisptr
    cdef int done_maxflow

    def __cinit__(self, int max_node_num = 128):
        self.done_maxflow = False
        self.thisptr = new MaxflowGraphDouble(max_node_num)

    def __dealloc__(self):
        del self.thisptr

    def from_py_object(self, object edge_list, int s, int t):
        self.thisptr.FromPyObject(edge_list, 0)
        self.thisptr.SetSourceSink(s, t)

    def max_preflow(self, int global_relabel_frequency=1, float tol=1e-6):
        self.done_maxflow = True
        return self.thisptr.MaxPreFlow(global_relabel_frequency, tol)

    def min_cut(self):
        if not self.done_maxflow:
            self.max_preflow()

        self.thisptr.MinCut()
        return self.thisptr.ToPythonMinCut()
