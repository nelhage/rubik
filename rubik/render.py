from io import StringIO

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

def render(cube):
  return Renderer(cube).render()
