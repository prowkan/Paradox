import os
import math
import struct

class Texel:

    def __init__(self):
        self.R = 0
        self.G = 0
        self.B = 0
        self.A = 0

class Color:

   def __init__(self, R = 0, G = 0, B = 0):
        self.R = R
        self.G = G
        self.B = B

def DistanceBetweenColor(Color1, Color2):
    return (Color1.R - Color2.R) * (Color1.R - Color2.R) + (Color1.G - Color2.G) * (Color1.G - Color2.G) + (Color1.B - Color2.B) * (Color1.B - Color2.B)

class CompressedTexelBlockBC1:

    def __init__(self):    
        self.Colors = [0, 0]
        self.Texels = [0, 0, 0, 0]

class CompressedTexelBlockBC5:

    def __init__(self):    
        self.Red = [0, 0]
        self.RedIndices = [0, 0, 0, 0, 0, 0]
        self.Green = [0, 0]
        self.GreenIndices = [0, 0, 0, 0, 0, 0]

class Texture:

    def __init__(self):
        self.TextureData = []

    def GenerateMipmaps(self):
        k = 1
        while k < self.MIPLevels:
            mipsize = max([self.Width, self.Height]) >> k
            y = 0
            self.TextureData.append([])
            while y < mipsize:
                self.TextureData[k].append([])
                x = 0
                while x < mipsize:
                    self.TextureData[k][y].append(Texel())
                    Red = self.TextureData[k - 1][2 * y][2 * x].R
                    Red = Red + self.TextureData[k - 1][2 * y][2 * x + 1].R
                    Red = Red + self.TextureData[k - 1][2 * y + 1][2 * x].R
                    Red = Red + self.TextureData[k - 1][2 * y + 1][2 * x + 1].R
                    Red = math.floor(Red / 4)
                    Green = self.TextureData[k - 1][2 * y][2 * x].G
                    Green = Green + self.TextureData[k - 1][2 * y][2 * x + 1].G
                    Green = Green + self.TextureData[k - 1][2 * y + 1][2 * x].G
                    Green = Green + self.TextureData[k - 1][2 * y + 1][2 * x + 1].G
                    Green = math.floor(Green / 4)
                    Blue = self.TextureData[k - 1][2 * y][2 * x].B
                    Blue = Blue + self.TextureData[k - 1][2 * y][2 * x + 1].B
                    Blue = Blue + self.TextureData[k - 1][2 * y + 1][2 * x].B
                    Blue = Blue + self.TextureData[k - 1][2 * y + 1][2 * x + 1].B
                    Blue = math.floor(Blue / 4)
                    Alpha = 255
                    self.TextureData[k][y][x].R = Red
                    self.TextureData[k][y][x].G = Green
                    self.TextureData[k][y][x].B = Blue
                    self.TextureData[k][y][x].A = Alpha
                    x = x + 1
                y = y + 1
            k = k + 1

    def SerializeBC1(self):
        TextureBytes = bytearray()
        TextureBytes.extend(int(0).to_bytes(2, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Width).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Height).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.MIPLevels).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.SRGB).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Compressed).to_bytes(4, byteorder='little', signed=False))
        k = 0
        while k < self.MIPLevels:
            mipsize = (max([self.Width, self.Height]) // 4) >> k
            y = 0
            while y < mipsize:
                x = 0
                while x < mipsize:
                    TextureBytes.extend(int(self.TextureData[k][y][x].Colors[0]).to_bytes(2, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Colors[1]).to_bytes(2, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Texels[0]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Texels[1]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Texels[2]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Texels[3]).to_bytes(1, byteorder='little', signed=False))
                    x = x + 1
                y = y + 1
            k = k + 1
        return TextureBytes

    def SerializeBC5(self):
        TextureBytes = bytearray()
        TextureBytes.extend(int(0).to_bytes(2, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Width).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Height).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.MIPLevels).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.SRGB).to_bytes(4, byteorder='little', signed=False))
        TextureBytes.extend(int(self.Compressed).to_bytes(4, byteorder='little', signed=False))
        k = 0
        while k < self.MIPLevels:
            mipsize = (max([self.Width, self.Height]) // 4) >> k
            y = 0
            while y < mipsize:
                x = 0
                while x < mipsize:
                    TextureBytes.extend(int(self.TextureData[k][y][x].Red[0]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Red[1]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[0]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[1]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[2]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[3]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[4]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].RedIndices[5]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Green[0]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].Green[1]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[0]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[1]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[2]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[3]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[4]).to_bytes(1, byteorder='little', signed=False))
                    TextureBytes.extend(int(self.TextureData[k][y][x].GreenIndices[5]).to_bytes(1, byteorder='little', signed=False))
                    x = x + 1
                y = y + 1
            k = k + 1
        return TextureBytes

def CompressTextureBC1(InputTexture):
    OutputTexture = Texture()
    OutputTexture.Width = InputTexture.Width
    OutputTexture.Height = InputTexture.Height
    OutputTexture.MIPLevels = InputTexture.MIPLevels
    OutputTexture.SRGB = InputTexture.SRGB
    OutputTexture.Compressed = 1
    k = 0
    while k < OutputTexture.MIPLevels:
        mipsize = (max([OutputTexture.Width, OutputTexture.Height]) // 4) >> k
        OutputTexture.TextureData.append([])
        y = 0
        while y < mipsize:
            x = 0
            OutputTexture.TextureData[k].append([])
            while x < mipsize:
                MinColor = Color(255, 255, 255)
                MaxColor = Color(0, 0, 0)
                Distance = -1.0
                j1max = 0
                i1max = 0
                j2max = 0
                i2max = 0
                for j1 in range(4):
                    for i1 in range(4):
                        color1 = Color(InputTexture.TextureData[k][4 * y + j1][4 * x + i1].R, InputTexture.TextureData[k][4 * y + j1][4 * x + i1].G, InputTexture.TextureData[k][4 * y + j1][4 * x + i1].B)
                        for j2 in range(4):
                            for i2 in range(4):
                                color2 = Color(InputTexture.TextureData[k][4 * y + j2][4 * x + i2].R, InputTexture.TextureData[k][4 * y + j2][4 * x + i2].G, InputTexture.TextureData[k][4 * y + j2][4 * x + i2].B)
                                TestDistance = DistanceBetweenColor(color1, color2)
                                if TestDistance > Distance:
                                    Distance = TestDistance
                                    j1max = j1
                                    i1max = i1
                                    j2max = j2
                                    i2max = i2
                MinColor = Color(InputTexture.TextureData[k][4 * y + j1max][4 * x + i1max].R, InputTexture.TextureData[k][4 * y + j1max][4 * x + i1max].G, InputTexture.TextureData[k][4 * y + j1max][4 * x + i1max].B)
                MaxColor = Color(InputTexture.TextureData[k][4 * y + j2max][4 * x + i2max].R, InputTexture.TextureData[k][4 * y + j2max][4 * x + i2max].G, InputTexture.TextureData[k][4 * y + j2max][4 * x + i2max].B)
                if (MinColor.R < MaxColor.R) or ((MinColor.R == MaxColor.R) and (MinColor.G < MaxColor.G)) or ((MinColor.R == MaxColor.R) and (MinColor.G == MaxColor.G) and (MinColor.B < MaxColor.B)):
                    TmpColor = MinColor
                    MinColor = MaxColor
                    MaxColor = TmpColor
                Colors = [Color(), Color(), Color(), Color()]
                Colors[0] = MaxColor
                Colors[1] = MinColor
                OutputTexture.TextureData[k][y].append(CompressedTexelBlockBC1())
                OutputTexture.TextureData[k][y][x].Colors[0] = OutputTexture.TextureData[k][y][x].Colors[0] | ((math.floor(Colors[0].R / 255.0 * 31.0) & 31) << 11)
                OutputTexture.TextureData[k][y][x].Colors[0] = OutputTexture.TextureData[k][y][x].Colors[0] | ((math.floor(Colors[0].G / 255.0 * 63.0) & 63) << 5)
                OutputTexture.TextureData[k][y][x].Colors[0] = OutputTexture.TextureData[k][y][x].Colors[0] | ((math.floor(Colors[0].B / 255.0 * 31.0) & 31))
                OutputTexture.TextureData[k][y][x].Colors[1] = OutputTexture.TextureData[k][y][x].Colors[1] | ((math.floor(Colors[1].R / 255.0 * 31.0) & 31) << 11)
                OutputTexture.TextureData[k][y][x].Colors[1] = OutputTexture.TextureData[k][y][x].Colors[1] | ((math.floor(Colors[1].G / 255.0 * 63.0) & 63) << 5)
                OutputTexture.TextureData[k][y][x].Colors[1] = OutputTexture.TextureData[k][y][x].Colors[1] | ((math.floor(Colors[1].B / 255.0 * 31.0) & 31))
                if OutputTexture.TextureData[k][y][x].Colors[0] > OutputTexture.TextureData[k][y][x].Colors[1]:
                    Colors[2].R = 2 * Colors[0].R / 3 + Colors[1].R / 3
                    Colors[2].G = 2 * Colors[0].G / 3 + Colors[1].G / 3
                    Colors[2].B = 2 * Colors[0].B / 3 + Colors[1].B / 3
                    Colors[3].R = Colors[0].R / 3 + 2 * Colors[1].R / 3
                    Colors[3].G = Colors[0].G / 3 + 2 * Colors[1].G / 3
                    Colors[3].B = Colors[0].B / 3 + 2 * Colors[1].B / 3
                else:
                    Colors[2].R = Colors[0].R / 2 + Colors[1].R / 2
                    Colors[2].G = Colors[0].G / 2 + Colors[1].G / 2
                    Colors[2].B = Colors[0].B / 2 + Colors[1].B / 2
                    Colors[3] = Color(0, 0, 0)
                for j in range(4):
                    for i in range(4):
                        TexelColor = Color(InputTexture.TextureData[k][4 * y + j][4 * x + i].R, InputTexture.TextureData[k][4 * y + j][4 * x + i].G, InputTexture.TextureData[k][4 * y + j][4 * x + i].B)
                        Dist = DistanceBetweenColor(TexelColor, Colors[0])
                        ArgMin = 0
                        for x1 in range(4):
                            NewDist = DistanceBetweenColor(TexelColor, Colors[x1])
                            if NewDist < Dist:
                                Dist = NewDist
                                ArgMin = x1
                            
                        OutputTexture.TextureData[k][y][x].Texels[j] = OutputTexture.TextureData[k][y][x].Texels[j] | ((ArgMin & 3) << (2 * i))
                        i = i + 1
                    j = j + 1
                x = x + 1
            y = y + 1
        k = k + 1
    return OutputTexture

def CompressTextureBC5(InputTexture):
    OutputTexture = Texture()
    OutputTexture.Width = InputTexture.Width
    OutputTexture.Height = InputTexture.Height
    OutputTexture.MIPLevels = InputTexture.MIPLevels
    OutputTexture.SRGB = 0
    OutputTexture.Compressed = 1
    k = 0
    while k < OutputTexture.MIPLevels:
        mipsize = (max([OutputTexture.Width, OutputTexture.Height]) // 4) >> k
        OutputTexture.TextureData.append([])
        y = 0
        while y < mipsize:
            x = 0
            OutputTexture.TextureData[k].append([])
            while x < mipsize:
                MinRed = 255
                MaxRed = 0
                MinGreen = 255
                MaxGreen = 0
                for j in range(4):
                    for i in range(4):
                        if InputTexture.TextureData[k][4 * y + j][4 * x + i].R < MinRed:
                            MinRed = InputTexture.TextureData[k][4 * y + j][4 * x + i].R
                        if InputTexture.TextureData[k][4 * y + j][4 * x + i].R > MaxRed:
                            MaxRed = InputTexture.TextureData[k][4 * y + j][4 * x + i].R
                        if InputTexture.TextureData[k][4 * y + j][4 * x + i].G < MinGreen:
                            MinGreen = InputTexture.TextureData[k][4 * y + j][4 * x + i].G
                        if InputTexture.TextureData[k][4 * y + j][4 * x + i].G > MaxGreen:
                            MaxGreen = InputTexture.TextureData[k][4 * y + j][4 * x + i].G
                RedColorTable = [0, 0, 0, 0, 0, 0, 0, 0]
                GreenColorTable = [0, 0, 0, 0, 0, 0, 0, 0]
                RedColorTable[0] = MaxRed
                RedColorTable[1] = MinRed
                GreenColorTable[0] = MaxGreen
                GreenColorTable[1] = MinGreen
                OutputTexture.TextureData[k][y].append(CompressedTexelBlockBC5())
                OutputTexture.TextureData[k][y][x].Red[0] = RedColorTable[0]
                OutputTexture.TextureData[k][y][x].Red[1] = RedColorTable[1]
                OutputTexture.TextureData[k][y][x].Green[0] = GreenColorTable[0]
                OutputTexture.TextureData[k][y][x].Green[1] = GreenColorTable[1]
                
                if RedColorTable[0] > RedColorTable[1]:
                    RedColorTable[2] = (6.0 * RedColorTable[0] + 1.0 * RedColorTable[1]) / 7.0
                    RedColorTable[3] = (5.0 * RedColorTable[0] + 2.0 * RedColorTable[1]) / 7.0
                    RedColorTable[4] = (4.0 * RedColorTable[0] + 3.0 * RedColorTable[1]) / 7.0
                    RedColorTable[5] = (3.0 * RedColorTable[0] + 4.0 * RedColorTable[1]) / 7.0
                    RedColorTable[6] = (2.0 * RedColorTable[0] + 5.0 * RedColorTable[1]) / 7.0
                    RedColorTable[7] = (1.0 * RedColorTable[0] + 6.0 * RedColorTable[1]) / 7.0
                else:
                    RedColorTable[2] = (4.0 * RedColorTable[0] + 1.0 * RedColorTable[1]) / 5.0
                    RedColorTable[3] = (3.0 * RedColorTable[0] + 2.0 * RedColorTable[1]) / 5.0
                    RedColorTable[4] = (2.0 * RedColorTable[0] + 3.0 * RedColorTable[1]) / 5.0
                    RedColorTable[5] = (1.0 * RedColorTable[0] + 4.0 * RedColorTable[1]) / 5.0
                    RedColorTable[6] = 0.0
                    RedColorTable[7] = 255.0

                if GreenColorTable[0] > GreenColorTable[1]:
                    GreenColorTable[2] = (6.0 * GreenColorTable[0] + 1.0 * GreenColorTable[1]) / 7.0
                    GreenColorTable[3] = (5.0 * GreenColorTable[0] + 2.0 * GreenColorTable[1]) / 7.0
                    GreenColorTable[4] = (4.0 * GreenColorTable[0] + 3.0 * GreenColorTable[1]) / 7.0
                    GreenColorTable[5] = (3.0 * GreenColorTable[0] + 4.0 * GreenColorTable[1]) / 7.0
                    GreenColorTable[6] = (2.0 * GreenColorTable[0] + 5.0 * GreenColorTable[1]) / 7.0
                    GreenColorTable[7] = (1.0 * GreenColorTable[0] + 6.0 * GreenColorTable[1]) / 7.0
                else:
                    GreenColorTable[2] = (4.0 * GreenColorTable[0] + 1.0 * GreenColorTable[1]) / 5.0
                    GreenColorTable[3] = (3.0 * GreenColorTable[0] + 2.0 * GreenColorTable[1]) / 5.0
                    GreenColorTable[4] = (2.0 * GreenColorTable[0] + 3.0 * GreenColorTable[1]) / 5.0
                    GreenColorTable[5] = (1.0 * GreenColorTable[0] + 4.0 * GreenColorTable[1]) / 5.0
                    GreenColorTable[6] = 0.0
                    GreenColorTable[7] = 255.0

                RedIndices = [0, 0]
                GreenIndices = [0, 0]

                CurrentIndex = 0

                for j in range(4):
                    for i in range(4):

                        TexelColor = Color(InputTexture.TextureData[k][4 * y + j][4 * x + i].R, InputTexture.TextureData[k][4 * y + j][4 * x + i].G, InputTexture.TextureData[k][4 * y + j][4 * x + i].B)
                        
                        RedDist = abs(TexelColor.R - RedColorTable[0])
                        GreenDist = abs(TexelColor.G - GreenColorTable[0])
                        
                        RedArgMin = 0
                        GreenArgMin = 0

                        for x1 in range(8):

                            NewRedDist = abs(TexelColor.R - RedColorTable[x1])
                            NewGreenDist = abs(TexelColor.G - GreenColorTable[x1])

                            if NewRedDist < RedDist:
                                RedDist = NewRedDist
                                RedArgMin = x1

                            if NewGreenDist < GreenDist:
                                GreenDist = NewGreenDist
                                GreenArgMin = x1
                            
                        RedIndices[CurrentIndex // 8] = RedIndices[CurrentIndex // 8] | ((RedArgMin & 7) << (3 * (CurrentIndex % 8)))
                        GreenIndices[CurrentIndex // 8] = GreenIndices[CurrentIndex // 8] | ((GreenArgMin & 7) << (3 * (CurrentIndex % 8)))

                        CurrentIndex = CurrentIndex + 1

                        i = i + 1
                    j = j + 1

                RedIndicesBytes = [ int(RedIndices[0]).to_bytes(4, byteorder='little', signed=False), int(RedIndices[1]).to_bytes(4, byteorder='little', signed=False) ]
                GreenIndicesBytes = [ int(GreenIndices[0]).to_bytes(4, byteorder='little', signed=False), int(GreenIndices[1]).to_bytes(4, byteorder='little', signed=False) ]

                OutputTexture.TextureData[k][y][x].RedIndices[0] = RedIndicesBytes[0][0]
                OutputTexture.TextureData[k][y][x].RedIndices[1] = RedIndicesBytes[0][1]
                OutputTexture.TextureData[k][y][x].RedIndices[2] = RedIndicesBytes[0][2]
                OutputTexture.TextureData[k][y][x].RedIndices[3] = RedIndicesBytes[1][0]
                OutputTexture.TextureData[k][y][x].RedIndices[4] = RedIndicesBytes[1][1]
                OutputTexture.TextureData[k][y][x].RedIndices[5] = RedIndicesBytes[1][2]

                OutputTexture.TextureData[k][y][x].GreenIndices[0] = GreenIndicesBytes[0][0]
                OutputTexture.TextureData[k][y][x].GreenIndices[1] = GreenIndicesBytes[0][1]
                OutputTexture.TextureData[k][y][x].GreenIndices[2] = GreenIndicesBytes[0][2]
                OutputTexture.TextureData[k][y][x].GreenIndices[3] = GreenIndicesBytes[1][0]
                OutputTexture.TextureData[k][y][x].GreenIndices[4] = GreenIndicesBytes[1][1]
                OutputTexture.TextureData[k][y][x].GreenIndices[5] = GreenIndicesBytes[1][2]

                x = x + 1
            y = y + 1
        k = k + 1
    return OutputTexture

def GenerateCheckerTexture():
    OutputTexture = Texture()
    OutputTexture.Width = 512
    OutputTexture.Height = 512
    OutputTexture.MIPLevels = 8
    OutputTexture.SRGB = 1
    OutputTexture.Compressed = 0
    x = 0
    y = 0
    OutputTexture.TextureData.append([])
    while y < OutputTexture.Height:
        x = 0
        OutputTexture.TextureData[0].append([])
        while x < OutputTexture.Width:
            OutputTexture.TextureData[0][y].append(Texel())
            OutputTexture.TextureData[0][y][x].R = 0 if ((x // 64) + (y // 64)) % 2 > 0 else 255
            OutputTexture.TextureData[0][y][x].G = 0 if ((x // 64) + (y // 64)) % 2 > 0 else 255
            OutputTexture.TextureData[0][y][x].B = 192 if ((x // 64) + (y // 64)) % 2 > 0 else 255
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1
    OutputTexture.GenerateMipmaps()
    return OutputTexture

def GenerateNormalTexture():
    OutputTexture = Texture()
    OutputTexture.Width = 512
    OutputTexture.Height = 512
    OutputTexture.MIPLevels = 8
    OutputTexture.SRGB = 0
    OutputTexture.Compressed = 0
    x = 0
    y = 0
    OutputTexture.TextureData.append([])
    while y < OutputTexture.Height:
        x = 0
        OutputTexture.TextureData[0].append([])
        while x < OutputTexture.Width:
            OutputTexture.TextureData[0][y].append(Texel())
            NX = 0
            NY = 0
            NZ = 1
            Length = math.sqrt(NX * NX + NY * NY + NZ * NZ)
            NX = NX / Length
            NY = NY / Length
            NZ = NZ / Length
            NX = math.floor((0.5 * NX + 0.5) * 255)
            NY = math.floor((0.5 * NY + 0.5) * 255)
            NZ = math.floor((0.5 * NZ + 0.5) * 255)
            OutputTexture.TextureData[0][y][x].R = NX
            OutputTexture.TextureData[0][y][x].G = NY
            OutputTexture.TextureData[0][y][x].B = NZ
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1

    y = 128 - 28
    while y < 128 + 28:
        x = y
        while x < 512 - y:
            NX = 0
            NY = -1
            NZ = 1
            Length = math.sqrt(NX * NX + NY * NY + NZ * NZ)
            NX = NX / Length
            NY = NY / Length
            NZ = NZ / Length
            NX = math.floor((0.5 * NX + 0.5) * 255)
            NY = math.floor((0.5 * NY + 0.5) * 255)
            NZ = math.floor((0.5 * NZ + 0.5) * 255)
            OutputTexture.TextureData[0][y][x].R = NX
            OutputTexture.TextureData[0][y][x].G = NY
            OutputTexture.TextureData[0][y][x].B = NZ
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1

    y = 256 + 128 - 28
    while y < 256 + 128 + 28:
        x = 512 - y
        while x < y:
            NX = 0
            NY = 1
            NZ = 1
            Length = math.sqrt(NX * NX + NY * NY + NZ * NZ)
            NX = NX / Length
            NY = NY / Length
            NZ = NZ / Length
            NX = math.floor((0.5 * NX + 0.5) * 255)
            NY = math.floor((0.5 * NY + 0.5) * 255)
            NZ = math.floor((0.5 * NZ + 0.5) * 255)
            OutputTexture.TextureData[0][y][x].R = NX
            OutputTexture.TextureData[0][y][x].G = NY
            OutputTexture.TextureData[0][y][x].B = NZ
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1

    x = 128 - 28
    while x < 128 + 28:
        y = x
        while y < 512 - x:
            NX = -1
            NY = 0
            NZ = 1
            Length = math.sqrt(NX * NX + NY * NY + NZ * NZ)
            NX = NX / Length
            NY = NY / Length
            NZ = NZ / Length
            NX = math.floor((0.5 * NX + 0.5) * 255)
            NY = math.floor((0.5 * NY + 0.5) * 255)
            NZ = math.floor((0.5 * NZ + 0.5) * 255)
            OutputTexture.TextureData[0][y][x].R = NX
            OutputTexture.TextureData[0][y][x].G = NY
            OutputTexture.TextureData[0][y][x].B = NZ
            OutputTexture.TextureData[0][y][x].A = 255
            y = y + 1
        x = x + 1

    x = 256 + 128 - 28
    while x < 256 + 128 + 28:
        y = 512 - x
        while y < x:
            NX = 1
            NY = 0
            NZ = 1
            Length = math.sqrt(NX * NX + NY * NY + NZ * NZ)
            NX = NX / Length
            NY = NY / Length
            NZ = NZ / Length
            NX = math.floor((0.5 * NX + 0.5) * 255)
            NY = math.floor((0.5 * NY + 0.5) * 255)
            NZ = math.floor((0.5 * NZ + 0.5) * 255)
            OutputTexture.TextureData[0][y][x].R = NX
            OutputTexture.TextureData[0][y][x].G = NY
            OutputTexture.TextureData[0][y][x].B = NZ
            OutputTexture.TextureData[0][y][x].A = 255
            y = y + 1
        x = x + 1

    OutputTexture.GenerateMipmaps()
    return OutputTexture

def GenerateEmissiveTexture():
    OutputTexture = Texture()
    OutputTexture.Width = 512
    OutputTexture.Height = 512
    OutputTexture.MIPLevels = 8
    OutputTexture.SRGB = 1
    OutputTexture.Compressed = 0
    x = 0
    y = 0
    OutputTexture.TextureData.append([])
    while y < OutputTexture.Height:
        x = 0
        OutputTexture.TextureData[0].append([])
        while x < OutputTexture.Width:
            OutputTexture.TextureData[0][y].append(Texel())
            OutputTexture.TextureData[0][y][x].R = 0
            OutputTexture.TextureData[0][y][x].G = 0
            OutputTexture.TextureData[0][y][x].B = 0
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1
    x = 128 + 28
    y = 128 + 28
    while y < 256 + 128 - 28:
        x = 128 + 28
        while x < 256 + 128 - 28:
            OutputTexture.TextureData[0][y][x].R = 128
            OutputTexture.TextureData[0][y][x].G = 64
            OutputTexture.TextureData[0][y][x].B = 255
            OutputTexture.TextureData[0][y][x].A = 255
            x = x + 1
        y = y + 1

    OutputTexture.GenerateMipmaps()
    return OutputTexture

DiffuseTexture = GenerateCheckerTexture()
CompressedDiffuseTexture = CompressTextureBC1(DiffuseTexture)
DiffuseTextureData = CompressedDiffuseTexture.SerializeBC1()

NormalTexture = GenerateNormalTexture()
CompressedNormalTexture = CompressTextureBC5(NormalTexture)
NormalTextureData = CompressedNormalTexture.SerializeBC5()

EmissiveTexture = GenerateEmissiveTexture()
CompressedEmissiveTexture = CompressTextureBC1(EmissiveTexture)
EmissiveTextureData = CompressedEmissiveTexture.SerializeBC1()

os.chdir("D:/Paradox/Build/GameContent/Test")

i = 0
while i < 4000:
    f = open("T_Default_" + str(i) + "_D.dasset", "wb")
    f.write(DiffuseTextureData)
    f.close()
    f = open("T_Default_" + str(i) + "_N.dasset", "wb")
    f.write(NormalTextureData)
    f.close()
    f = open("T_Default_" + str(i) + "_E.dasset", "wb")
    f.write(EmissiveTextureData)
    f.close()
    i = i + 1

class Vector3:

    def __init__(self, X = 0.0, Y = 0.0, Z = 0.0):
        self.X = X
        self.Y = Y
        self.Z = Z

    def Serialize(self):
        Bytes = bytearray()
        Bytes.extend(struct.pack("f", self.X))
        Bytes.extend(struct.pack("f", self.Y))
        Bytes.extend(struct.pack("f", self.Z))
        return Bytes

class Vector2:

    def __init__(self, X = 0.0, Y = 0.0):
        self.X = X
        self.Y = Y

    def Serialize(self):
        Bytes = bytearray()
        Bytes.extend(struct.pack("f", self.X))
        Bytes.extend(struct.pack("f", self.Y))
        return Bytes

class Vertex:

    def __init__(self):
        self.Position = Vector3()
        self.TexCoord = Vector2()
        self.Normal = Vector3()
        self.Tangent = Vector3()
        self.Binormal = Vector3()

    def Serialize(self):
        Bytes = bytearray()
        Bytes.extend(self.Position.Serialize())
        Bytes.extend(self.TexCoord.Serialize())
        Bytes.extend(self.Normal.Serialize())
        Bytes.extend(self.Tangent.Serialize())
        Bytes.extend(self.Binormal.Serialize())
        return Bytes

VertexCount = 33 * 33 * 6 + 17 * 17 * 6 + 9 * 9 * 6 + 5 * 5 * 6 + 3 * 3 * 6
IndexCount = 32 * 32 * 6 * 6 + 16 * 16 * 6 * 6 + 8 * 8 * 6 * 6 + 4 * 4 * 6 * 6 + 2 * 2 * 6 * 6

Positions1 = [Vector3()] * VertexCount;
TexCoords1 = [Vector2()] * VertexCount;
TangentSpaces1 = [[]] * VertexCount;

Indices1 = [0] * IndexCount

Positions2 = [Vector3()] * VertexCount;
TexCoords2 = [Vector2()] * VertexCount;
TangentSpaces2 = [[]] * VertexCount;

Indices2 = [0] * IndexCount

def GenerateCubeMesh1(PositionsArray, TexCoordsArray, TangentSpacesArray, IndicesArray, VertexArrayOffset, IndexArrayOffset, CubeMeshSize):

    for i in range(CubeMeshSize + 1):
        for j in range(CubeMeshSize + 1):

            PositionsArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = Vector3(-1.0 + j * 2.0 / CubeMeshSize, 1.0 - i * 2.0 / CubeMeshSize, -1.0)
            TexCoordsArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 0.0, -1.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(1.0, 1.0 - i * 2.0 / CubeMeshSize, -1.0 + j * 2.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(0.0, 0.0, 1.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(1.0 - j * 2.0 / CubeMeshSize, 1.0 - i * 2.0 / CubeMeshSize, 1.0)
            TexCoordsArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 0.0, 1.0)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(-1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-1.0, 1.0 - i * 2.0 / CubeMeshSize, 1.0 - j * 2.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(-1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(0.0, 0.0, -1.0)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-1.0 + j * 2.0 / CubeMeshSize, 1.0, 1.0 - i * 2.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 1.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, 0.0, -1.0)

            PositionsArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-1.0 + j * 2.0 / CubeMeshSize, -1.0, -1.0 + i * 2.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, -1.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, 0.0, 1.0)


    if CubeMeshSize == 2:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(CubeMeshSize):
                for j in range(CubeMeshSize):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 4:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 1):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(3, 4):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(1, 3):
                for j in range(0, 1):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(1, 3):
                for j in range(3, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(1, 3):
                for j in range(1, 3):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 8:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 2):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(6, 8):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(2, 6):
                for j in range(0, 2):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(2, 6):
                for j in range(6, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(2, 6):
                for j in range(2, 6):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 16:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 4):
                for j in range(0, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(12, 16):
                for j in range(0, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(4, 12):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(4, 12):
                for j in range(12, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(4, 12):
                for j in range(4, 12):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 32:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 8):
                for j in range(0, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(24, 32):
                for j in range(0, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(8, 24):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(8, 24):
                for j in range(24, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(8, 24):
                for j in range(8, 24):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    else:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(CubeMeshSize):
                for j in range(CubeMeshSize):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
                
def GenerateCubeMesh2(PositionsArray, TexCoordsArray, TangentSpacesArray, IndicesArray, VertexArrayOffset, IndexArrayOffset, CubeMeshSize):

    for i in range(CubeMeshSize + 1):
        for j in range(CubeMeshSize + 1):

            PositionsArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = Vector3(-5.0 + j * 10.0 / CubeMeshSize, 1.0 - i * 2.0 / CubeMeshSize, -5.0)
            TexCoordsArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 0.0, -1.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(5.0, 1.0 - i * 2.0 / CubeMeshSize, -5.0 + j * 10.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(0.0, 0.0, 1.0)
            TangentSpacesArray[VertexArrayOffset + (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(5.0 - j * 10.0 / CubeMeshSize, 1.0 - i * 2.0 / CubeMeshSize, 5.0)
            TexCoordsArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 0.0, 1.0)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(-1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 2 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-5.0, 1.0 - i * 2.0 / CubeMeshSize, 5.0 - j * 10.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(-1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(0.0, 0.0, -1.0)
            TangentSpacesArray[VertexArrayOffset + 3 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, -1.0, 0.0)

            PositionsArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-5.0 + j * 10.0 / CubeMeshSize, 1.0, 5.0 - i * 10.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, 1.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 4 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, 0.0, -1.0)

            PositionsArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector3(-5.0 + j * 10.0 / CubeMeshSize, -1.0, -5.0 + i * 10.0 / CubeMeshSize)
            TexCoordsArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = Vector2(j * 1.0 / CubeMeshSize, i * 1.0 / CubeMeshSize)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j] = [Vector3()] * 3;
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][0] = Vector3(0.0, -1.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][1] = Vector3(1.0, 0.0, 0.0)
            TangentSpacesArray[VertexArrayOffset + 5 * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j][2] = Vector3(0.0, 0.0, 1.0)


    if CubeMeshSize == 2:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(CubeMeshSize):
                for j in range(CubeMeshSize):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 4:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 1):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(3, 4):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(1, 3):
                for j in range(0, 1):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(1, 3):
                for j in range(3, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(1, 3):
                for j in range(1, 3):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 8:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 2):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(6, 8):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(2, 6):
                for j in range(0, 2):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(2, 6):
                for j in range(6, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(2, 6):
                for j in range(2, 6):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 16:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 4):
                for j in range(0, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(12, 16):
                for j in range(0, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(4, 12):
                for j in range(0, 4):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(4, 12):
                for j in range(12, 16):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(4, 12):
                for j in range(4, 12):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    elif CubeMeshSize == 32:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(0, 8):
                for j in range(0, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(24, 32):
                for j in range(0, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(8, 24):
                for j in range(0, 8):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

            for i in range(8, 24):
                for j in range(24, 32):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

        for k in [5, 4, 0, 1, 2, 3]:
            for i in range(8, 24):
                for j in range(8, 24):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1
    else:
        ArrayIndex = 0            
        for k in range(6):
            for i in range(CubeMeshSize):
                for j in range(CubeMeshSize):

                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * i + j + 1
                    ArrayIndex = ArrayIndex + 1
                    IndicesArray[IndexArrayOffset + ArrayIndex] = k * (CubeMeshSize + 1) * (CubeMeshSize + 1) + (CubeMeshSize + 1) * (i + 1) + j + 1
                    ArrayIndex = ArrayIndex + 1

GenerateCubeMesh1(Positions1, TexCoords1, TangentSpaces1, Indices1, 0, 0, 32)
GenerateCubeMesh1(Positions1, TexCoords1, TangentSpaces1, Indices1, 33 * 33 * 6, 32 * 32 * 6 * 6, 16)
GenerateCubeMesh1(Positions1, TexCoords1, TangentSpaces1, Indices1, (33 * 33 + 17 * 17) * 6, (32 * 32 + 16 * 16) * 6 * 6, 8)
GenerateCubeMesh1(Positions1, TexCoords1, TangentSpaces1, Indices1, (33 * 33 + 17 * 17 + 9 * 9) * 6, (32 * 32 + 16 * 16 + 8 * 8) * 6 * 6, 4)
GenerateCubeMesh1(Positions1, TexCoords1, TangentSpaces1, Indices1, (33 * 33 + 17 * 17 + 9 * 9 + 5 * 5) * 6, (32 * 32 + 16 * 16 + 8 * 8 + 4 * 4) * 6 * 6, 2)

GenerateCubeMesh2(Positions2, TexCoords2, TangentSpaces2, Indices2, 0, 0, 32)
GenerateCubeMesh2(Positions2, TexCoords2, TangentSpaces2, Indices2, 33 * 33 * 6, 32 * 32 * 6 * 6, 16)
GenerateCubeMesh2(Positions2, TexCoords2, TangentSpaces2, Indices2, (33 * 33 + 17 * 17) * 6, (32 * 32 + 16 * 16) * 6 * 6, 8)
GenerateCubeMesh2(Positions2, TexCoords2, TangentSpaces2, Indices2, (33 * 33 + 17 * 17 + 9 * 9) * 6, (32 * 32 + 16 * 16 + 8 * 8) * 6 * 6, 4)
GenerateCubeMesh2(Positions2, TexCoords2, TangentSpaces2, Indices2, (33 * 33 + 17 * 17 + 9 * 9 + 5 * 5) * 6, (32 * 32 + 16 * 16 + 8 * 8 + 4 * 4) * 6 * 6, 2)

CubeMeshData1 = bytearray()
CubeMeshData1.extend(int(2).to_bytes(2, byteorder='little', signed=False))
CubeMeshData1.extend(int(VertexCount).to_bytes(4, byteorder='little', signed=False))
CubeMeshData1.extend(int(IndexCount).to_bytes(4, byteorder='little', signed=False))

CubeMeshData2 = bytearray()
CubeMeshData2.extend(int(2).to_bytes(2, byteorder='little', signed=False))
CubeMeshData2.extend(int(VertexCount).to_bytes(4, byteorder='little', signed=False))
CubeMeshData2.extend(int(IndexCount).to_bytes(4, byteorder='little', signed=False))

for Pos in Positions1:
    CubeMeshData1.extend(Pos.Serialize())
for Tc in TexCoords1:
    CubeMeshData1.extend(Tc.Serialize())
for TS in TangentSpaces1:
    CubeMeshData1.extend(TS[0].Serialize())
    CubeMeshData1.extend(TS[1].Serialize())
    CubeMeshData1.extend(TS[2].Serialize())

for Idx in Indices1:
    CubeMeshData1.extend(int(Idx).to_bytes(2, byteorder='little', signed=False))

for Pos in Positions2:
    CubeMeshData2.extend(Pos.Serialize())
for Tc in TexCoords2:
    CubeMeshData2.extend(Tc.Serialize())
for TS in TangentSpaces2:
    CubeMeshData2.extend(TS[0].Serialize())
    CubeMeshData2.extend(TS[1].Serialize())
    CubeMeshData2.extend(TS[2].Serialize())

for Idx in Indices2:
    CubeMeshData2.extend(int(Idx).to_bytes(2, byteorder='little', signed=False))

os.chdir("D:/Paradox/Build/GameContent/Test")

i = 0
while i < 4000:
    f = open("SM_Cube_" + str(i) + ".dasset", "wb")
    if i % 2 == 0:
        f.write(CubeMeshData1)
    else:
        f.write(CubeMeshData2)
    f.close()
    i = i + 1

os.chdir("D:/Paradox/Build/GameContent/Test")
i = 0
while i < 4000:
    f = open("M_Standart_" + str(i) + ".dasset", "wb")
    f.write(int(1).to_bytes(2, byteorder='little', signed=False))
    f.write(int(3).to_bytes(2, byteorder='little', signed=False))
    f.write(("Test.T_Default_" + str(i) + "_D").encode())
    f.write(b'\0')
    f.write(("Test.T_Default_" + str(i) + "_N").encode())
    f.write(b'\0')
    f.write(("Test.T_Default_" + str(i) + "_E").encode())
    f.write(b'\0')
    f.close()
    i = i + 1