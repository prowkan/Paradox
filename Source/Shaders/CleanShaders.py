import os
import glob

FileList = glob.glob("./../../Build/GameContent/Shaders/ShaderModel50/*.*")

for FilePath in FileList:
    os.remove(FilePath);

FileList = glob.glob("./../../Build/GameContent/Shaders/ShaderModel51/*.*")

for FilePath in FileList:
    os.remove(FilePath);