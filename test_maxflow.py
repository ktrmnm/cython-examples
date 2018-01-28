import networkx as nx
from exmodule import CythonMaxflowGraph, digraph_to_edge_list

G = nx.DiGraph()
G.add_edge('x','a', capacity=3.0)
G.add_edge('x','b', capacity=1.0)
G.add_edge('a','c', capacity=3.0)
G.add_edge('b','c', capacity=5.0)
G.add_edge('b','d', capacity=4.0)
G.add_edge('d','e', capacity=2.0)
G.add_edge('c','y', capacity=2.0)
G.add_edge('e','y', capacity=3.0)

G = nx.convert_node_labels_to_integers(G)
n = len(G)
s = 0
t = n - 1

cy_graph = CythonMaxflowGraph(max_node_num=n**2)
cy_graph.from_py_object(digraph_to_edge_list(G), s, t)

print(G.edges.data('capacity'))

print('NetworxX:')
print(nx.minimum_cut(G, s, t))

print('Cython:')
print(cy_graph.min_cut())
