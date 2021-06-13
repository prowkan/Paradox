import os
import struct

os.chdir("D:/Paradox/Build/GameContent/Maps")

f = open("000.dmap", "wb")

f.write(int(30000).to_bytes(4, 'little'))

ResourceCounter = 0

i = -50
j = -50

while i < 50:
    j = -50
    while j < 50:

        Bytes = bytearray()
        Bytes.extend("StaticMeshEntity".encode())
        Bytes += b'\0'

        Bytes.extend(int(3).to_bytes(4, 'little'))

        Bytes.extend("TransformComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", i * 5.0 + 2.5))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", j * 5.0 + 2.5))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))

        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        
        Bytes.extend("BoundingBoxComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        
        Bytes.extend(struct.pack("f", 2.0))
        Bytes.extend(struct.pack("f", 2.0))
        Bytes.extend(struct.pack("f", 2.0))

        Bytes.extend("StaticMeshComponent".encode())
        Bytes += b'\0'

        Bytes.extend(("Test.SM_Cube_" + str(ResourceCounter)).encode())
        Bytes += b'\0'
        Bytes.extend(("Test.M_Standart_" + str(ResourceCounter)).encode())
        Bytes += b'\0'
        
        ResourceCounter = (ResourceCounter + 1) % 4000

        Bytes.extend("StaticMeshEntity".encode())
        Bytes += b'\0'

        Bytes.extend(int(3).to_bytes(4, 'little'))

        Bytes.extend("TransformComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", i * 10.0 + 5.0))
        Bytes.extend(struct.pack("f", -2.0))
        Bytes.extend(struct.pack("f", j * 10.0 + 5.0))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))

        Bytes.extend(struct.pack("f", 5.0))
        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 5.0))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))

        Bytes.extend("BoundingBoxComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        
        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))

        Bytes.extend("StaticMeshComponent".encode())
        Bytes += b'\0'

        Bytes.extend(("Test.SM_Cube_" + str(ResourceCounter)).encode())
        Bytes += b'\0'
        Bytes.extend(("Test.M_Standart_" + str(ResourceCounter)).encode())
        Bytes += b'\0'

        ResourceCounter = (ResourceCounter + 1) % 4000

        Bytes.extend("PointLightEntity".encode())
        Bytes += b'\0'

        Bytes.extend(int(2).to_bytes(4, 'little'))

        Bytes.extend("TransformComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", i * 10.0 + 5.0))
        Bytes.extend(struct.pack("f", 1.5))
        Bytes.extend(struct.pack("f", j * 10.0 + 5.0))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))

        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))
        Bytes.extend(struct.pack("f", 1.0))

        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))
        Bytes.extend(struct.pack("f", 0.0))

        Bytes.extend("PointLightComponent".encode())
        Bytes += b'\0'

        Bytes.extend(struct.pack("f", 10.0))
        Bytes.extend(struct.pack("f", 5.0))

        Bytes.extend(struct.pack("f", (i + 51) / 100.0))
        Bytes.extend(struct.pack("f", 0.1))
        Bytes.extend(struct.pack("f", (j + 51) / 100.0))

        f.write(Bytes)
        
        j = j + 1
    i = i + 1

f.close();