import rubik
import numpy as np

moves = [
    rubik.Rotation.R,
    rubik.Rotation.L,
    rubik.Rotation.U,
    rubik.Rotation.D,
    rubik.Rotation.F,
    rubik.Rotation.B,
]

moves += [m.invert() for m in moves]

def coord(align, perm):
    return 12*align+perm


adj = np.ndarray((24, 24))
adj.fill(np.inf)

for m in moves:
    for i,j in enumerate(m.edge_perm):
        adj[coord(0, i)][coord(m.edge_align[i], j)] = 1
        adj[coord(1, i)][coord(not m.edge_align[i], j)] = 1

print(adj)

for k in range(24):
    adj = np.minimum(adj, adj[np.newaxis,k,:] + adj[:,k,np.newaxis])

print(adj)
