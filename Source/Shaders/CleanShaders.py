import os
import glob

FileList = glob.glob("./../../Build/GameContent/Shaders/*.*")

for FilePath in FileList:
    os.remove(FilePath);