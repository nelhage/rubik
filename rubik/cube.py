from io import StringIO

class Cube(object):
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
      (6,  0), (10, 0), (5, 0),
      (11, 0),  O,      (9, 0),
      (7,  0), (8,  0), (4, 0),
    ] ,
    'L': [
      (0, 1), (0, 1), (3, 2),
      (4, 1),  G,     (7, 1),
      (4, 2), (8, 1), (7, 1),
    ],
    'R': [
      (2, 1), (2, 1),  (1, 2),
      (6, 0),  B,      (5, 0),
      (6, 2), (10, 1), (5, 1)
    ],
    'F': [
      (3, 1), (3,  1), (2, 2),
      (7, 0),  Y,      (6, 1),
      (7, 2), (11, 1), (6, 1)
    ],
    'B': [
      (1, 1), (1, 1), (0, 2),
      (5, 1),  W,     (4, 0),
      (5, 2), (9, 1), (4, 1)
    ],
  }

  def __init__(self):
    self.edge_perm  = list(range(12))
    self.edge_align = [0] * 12
    self.corner_perm  = list(range(8))
    self.corner_align = [0] * 8

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

class Renderer(object):
  def __init__(self, cube):
    self.cube = cube
    self.buffer = StringIO()

  COLORMAP   = [9, 10, 15, 12, 11, 3]
  COLORCHARS = "RGWBYO"

  def setcolor(self, color):
    if color < 8:
      return "\x1b[{}m".format(30+color)
    else:
      return "\x1b[1;{}m".format(30+color-8)

  def resetcolor(self):
    return "\x1b[0m"

  def facelet(self, face, i, j):
    color = self.cube.facelet_color(face, 3*i+j)
    return " " + self.setcolor(self.COLORMAP[color]) + self.COLORCHARS[color] + self.resetcolor() + " "

  def render(self):
    if self.buffer.tell() != 0:
      raise ArgumentError("render can only be called once")

    # B
    self.buffer.write("            +===+===+===+\n")
    for i in range(3):
      self.buffer.write("            |")
      for j in range(3):
        self.buffer.write(self.facelet('B', i, j))
        if j != 2:
          self.buffer.write(" ")

      self.buffer.write("|\n")
      if i != 2:
        self.buffer.write("            + - + - + - +\n")

    # L U R D
    self.buffer.write("+===+===+===+===+===+===+===+===+===+===+===+===+\n")
    for i in range(3):
      for face in "LURD":
        self.buffer.write("|")
        for j in range(3):
          self.buffer.write(self.facelet(face, i, j))
          if j != 2:
            self.buffer.write(' ')
      self.buffer.write("|\n")
      if i != 2:
        self.buffer.write("+---+---+---+---+---+---+---+---+---+---+---+---+\n")
    self.buffer.write("+===+===+===+===+===+===+===+===+===+===+===+===+\n")

    # F
    for i in range(3):
      self.buffer.write("            |")
      for j in range(3):
        self.buffer.write(self.facelet('F', i, j))
        if j != 2:
          self.buffer.write(" ")

      self.buffer.write("|\n")
      if i != 2:
        self.buffer.write("            + - + - + - +\n")
    self.buffer.write("            +===+===+===+\n")
    return self.buffer.getvalue()
