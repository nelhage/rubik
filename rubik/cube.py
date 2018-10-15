import attr

@attr.s(frozen=True, slots=True, cmp=True)
class Cube(object):
  edge_perm    = attr.ib(default = attr.Factory(lambda: tuple(range(12))))
  edge_align   = attr.ib(default = attr.Factory(lambda: (0,) * 12))
  corner_perm  = attr.ib(default = attr.Factory(lambda: tuple(range(8))))
  corner_align = attr.ib(default = attr.Factory(lambda: (0,) * 8))

  @edge_perm.validator
  def _check_edge_perm(self, attr, value):
    if not isinstance(value, tuple) or len(value) != 12:
      raise TypeError("edge_perm must be a 12-tuple")
    if set(value) != set(range(12)):
      raise ValueError("edge_param must be a permutation on (0, 12]. Got: {}".format(value))

  @edge_align.validator
  def _check_edge_align(self, attr, value):
    if not isinstance(value, tuple) or len(value) != 12:
      raise TypeError("edge_align must be a 12-tuple")
    if [v for v in value if v not in (0, 1)]:
      raise ValueError("elements of edge_align must be in {0, 1}. Got: {}".format(value))

  @corner_perm.validator
  def _check_corner_perm(self, attr, value):
    if not isinstance(value, tuple) or len(value) != 8:
      raise TypeError("corner_perm must be a 8-tuple")
    if set(value) != set(range(8)):
      raise ValueError("corner_param must be a permutation on (0, 8]. Got: {}".format(value))

  @corner_align.validator
  def _check_corner_align(self, attr, value):
    if not isinstance(value, tuple) or len(value) != 8:
      raise TypeError("corner_align must be a 8-tuple")
    if [v for v in value if v not in (0, 1, 2)]:
      raise ValueError("elements of corner_align must be in {0, 1, 2}. Got: {}".format(value))

  R,G,W,B,Y,O = range(6)

  EDGE_COLORS = (
    (R, G),
    (R, W),
    (R, B),
    (R, Y),
    (W, G),
    (B, W),
    (B, Y),
    (Y, G),
    (O, G),
    (O, W),
    (O, B),
    (O, Y),
  )

  CORNER_COLORS = (
    (R, G, W),
    (R, W, B),
    (R, B, Y),
    (R, Y, G),
    (O, W, G),
    (O, B, W),
    (O, Y, B),
    (O, G, Y),
  )

  FACE_MAP = {
    'U': [
      (0, 0), (1, 0), (1, 0),
      (0, 0),  R,     (2, 0),
      (3, 0), (3, 0), (2, 0),
    ],
    'D': [
      (5, 0),  (9, 0),  (4, 0),
      (10, 0), O,       (8,  0),
      (6,  0), (11, 0), (7,  0),
    ] ,
    'L': [
      (4, 2), (4, 1), (0, 1),
      (8, 1), G,      (0, 1),
      (7, 1), (7, 1), (3, 2),
    ],
    'R': [
      (1, 2), (5, 0), (5, 1),
      (2, 1), B,      (10, 1),
      (2, 1), (6, 0), (6, 2),
    ],
    'F': [
      (3, 1), (3,  1), (2, 2),
      (7, 0),  Y,      (6, 1),
      (7, 2), (11, 1), (6, 1)
    ],
    'B': [
      (4, 1), (9, 1), (5, 2),
      (4, 0),  W,     (5, 1),
      (0, 2), (1, 1), (1, 1),
    ],
  }

  def facelet_color(self, face, facelet):
    facemap = self.FACE_MAP[face]
    if facelet in [0, 2, 6, 8]:
      idx, align = facemap[facelet]
      cubie = self.corner_perm[idx]
      return self.CORNER_COLORS[cubie][(align + self.corner_align[idx]) % 3]
    elif facelet in [1, 3, 5, 7]:
      idx, align = facemap[facelet]
      cubie = self.edge_perm[idx]
      return self.EDGE_COLORS[cubie][(align + self.edge_align[idx]) % 2]
    else:
      return facemap[facelet]

  def apply(self, other):
    edge_perm  = [self.edge_perm[i] for i in other.edge_perm]
    edge_align = [
      (self.edge_align[i]+other.edge_align[j])%2 for j,i in enumerate(other.edge_perm)
    ]
    corner_perm  = [self.corner_perm[i] for i in other.corner_perm]
    corner_align = [
      (self.corner_align[i]+other.corner_align[j])%3 for j,i in enumerate(other.corner_perm)
    ]
    return Cube(
      tuple(edge_perm),
      tuple(edge_align),
      tuple(corner_perm),
      tuple(corner_align),
    )

  def invert(self):
    edge_perm = [0]*12
    for i, j in enumerate(self.edge_perm):
      edge_perm[j] = i
    edge_align = [0]*12
    for i, j in enumerate(edge_perm):
      edge_align[i] = (2 - self.edge_align[j]) % 2

    corner_perm = [0]*8
    for i, j in enumerate(self.corner_perm):
      corner_perm[j] = i
    corner_align = [0]*8
    for i, j in enumerate(corner_perm):
      corner_align[i] = (3 - self.corner_align[j]) % 3

    return Cube(
      tuple(edge_perm),
      tuple(edge_align),
      tuple(corner_perm),
      tuple(corner_align),
    )

class Rotation(object):
  def _cube(edge_perm, edge_align, corner_perm, corner_align):
    for i, j in enumerate(edge_perm):
      if j == None:
        edge_perm[i] = i
    for i, j in enumerate(corner_perm):
      if j == None:
        corner_perm[i] = i
    return Cube(tuple(edge_perm),
                tuple(edge_align),
                tuple(corner_perm),
                tuple(corner_align))

  _ = None

  L = _cube(
    edge_perm    = [4, _, _, _, 8, _, _, 0, 7, _, _, _],
    edge_align   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    corner_perm  = [4, _, _, 0, 7, _, _, 3],
    corner_align = [1, 0, 0, 2, 2, 0, 0, 1],
  )
  L2 = L.apply(L)
  Linv = L.invert()

  R = _cube(
    edge_perm    = [_, _, 6, _, _, 2, 10, _, _, _, 5, _],
    edge_align   = [0, 0, 1, 0, 0, 1, 1,  0, 0, 0, 1, 0],
    corner_perm  = [_, 2, 6, _, _, 1, 5, _],
    corner_align = [0, 2, 1, 0, 0, 1, 2, 0],
  )
  R2 = R.apply(R)
  Rinv = R.invert()

  U = _cube(
    edge_perm    = [3, 0, 1, 2, _, _, _, _, _, _, _, _],
    edge_align   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    corner_perm  = [3, 0, 1, 2, _, _, _, _],
    corner_align = [0, 0, 0, 0, 0, 0, 0, 0],
  )
  U2 = U.apply(U)
  Uinv = U.invert()

  D = _cube(
    edge_perm    = [_, _, _, _, _, _, _, _, 9, 10, 11, 8],
    edge_align   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0],
    corner_perm  = [_, _, _, _, 5, 6, 7, 4],
    corner_align = [0, 0, 0, 0, 0, 0, 0, 0],
  )
  D2 = D.apply(D)
  Dinv = D.invert()

  F = _cube(
    edge_perm    = [_, _, _, 7, _, _, 3, 11, _, _, _, 6],
    edge_align   = [0, 0, 0, 1, 0, 0, 0, 1,  0, 0, 0, 0],
    corner_perm  = [_, _, 3, 7, _, _, 2, 6],
    corner_align = [0, 0, 2, 1, 0, 0, 1, 2],
  )
  F2 = F.apply(F)
  Finv = F.invert()

  B = _cube(
    edge_perm    = [_, 5, _, _, 1, 9, _, _, _, 4, _, _],
    edge_align   = [0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0],
    corner_perm  = [1, 5, _, _, 0, 4, _, _],
    corner_align = [2, 1, 0, 0, 1, 2, 0, 0],
  )
  B2 = B.apply(B)
  Binv = B.invert()

  del(_cube)
  del(_)

NOTATION = {
  "L": Rotation.L,
  "L2": Rotation.L2,
  "L'": Rotation.Linv,

  "R": Rotation.R,
  "R2": Rotation.R2,
  "R'": Rotation.Rinv,

  "F": Rotation.F,
  "F2": Rotation.F2,
  "F'": Rotation.Finv,

  "B": Rotation.B,
  "B2": Rotation.B2,
  "B'": Rotation.Binv,

  "U": Rotation.U,
  "U2": Rotation.U2,
  "U'": Rotation.Uinv,

  "D": Rotation.D,
  "D2": Rotation.D2,
  "D'": Rotation.Dinv,
}

def algorithm(txt):
  moves = [NOTATION[move] for move in txt.split()]
  cube = Cube()
  for move in moves:
    cube = cube.apply(move)
  return cube

# superflip: U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2
