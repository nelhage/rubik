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

def adjacency(moves, nperm, nalign):
    def coord(a, p):
        return a*nperm+p

    dim = nperm*nalign
    adj = np.ndarray((dim, dim), dtype=np.uint32)
    adj.fill(1000)

    for (perm, align) in moves:
        for i,j in enumerate(perm):
            for n in range(nalign):
                adj[coord(n, i)][coord((n+align[i])%nalign, j)] = 1

    for k in range(dim):
        adj = np.minimum(adj, adj[np.newaxis,k,:] + adj[:,k,np.newaxis])
    return adj


print("# edges")
print(adjacency(
    [(m.edge_perm, m.edge_align) for m in moves],
    12, 2))

print("# corners")
print(adjacency(
    [(m.corner_perm, m.corner_align) for m in moves],
    8, 3))
