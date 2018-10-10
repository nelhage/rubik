import rubik


def test_colors():
  assert len(set(rubik.Cube.EDGE_COLORS)) == len(rubik.Cube.EDGE_COLORS)
  assert len(set(rubik.Cube.CORNER_COLORS)) == len(rubik.Cube.CORNER_COLORS)

def test_facemap():
  edges = []
  corners = []
  corner_idx = [0, 2, 6, 8]
  edge_idx   = [1, 3, 5, 7]
  for face in rubik.Cube.FACE_MAP.values():
    edges.extend([face[i] for i in edge_idx])
    corners.extend([face[i] for i in corner_idx])
  assert (
    sorted(edges) ==
    [(i, j) for i in range(12) for j in range(2)]
  )
  assert (
    sorted(corners) ==
    [(i, j) for i in range(8) for j in range(3)]
  )
