#!/usr/bin/python

graph = {
    1: [2,5],
    2: [1,3],
    3: [6,4,2],
    4: [3,6,7],
    5: [6,1],
    6: [5,3,4],
    7: [4,8],
    8: [7],
    9: [10],
    10:[9],
    11:[],
    }


def bidirectional_search(graph, a_node, b_node):
    def build_next_level(paths, last_level, graph):
        new_level = []
        for pnode in last_level:
            for nnode in graph[pnode]:
                if nnode not in paths:
                    paths[nnode]=pnode
                    new_level.append(nnode)
        return new_level
    
    middle_nodes = []
    
    a_paths = { a_node:None } # node_no->prev_node
    b_paths = { b_node:None } #
    a_last_level = [a_node]
    b_last_level = [b_node]
    
    while len(a_last_level) and len(b_last_level):
        # get shared nodes
        a_nodes = set(a_paths.keys())
        b_nodes = set(b_paths.keys())
        middle_nodes = a_nodes.intersection(b_nodes)
        if middle_nodes: break
        # build next level/depth
        if len(a_nodes) <= len(b_nodes):
            a_last_level = build_next_level(a_paths, a_last_level, graph)
        else:
            b_last_level = build_next_level(b_paths, b_last_level, graph)
    else: # exited without break, empty path
        return []
    
    # build the result
    
    middle = list(middle_nodes)[0]
    #from middle to a
    res = []
    c = middle
    while c != a_node:
        c = a_paths[c]
        res.insert(0,c)
    
    res.append(middle)
    
    #from middle to b
    c = middle
    while c != b_node:
        c = b_paths[c]
        res.append(c)
    
    return res


print "%r" % bidirectional_search(graph, 4,8)