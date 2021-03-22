import os
import glob

#FileList = glob.glob("F:/Paradox/Build/GameContent/Objects/*.*")

class FileRecord:

    def __init__(self):
        self.FileName = ""
        self.FileSize = 0
        self.FileOffset = 0

    def Serialize(self):
        Bytes = bytearray()
        Bytes.extend(self.FileName.encode())
        Bytes += b'\0' * (1024 - len(self.FileName))
        Bytes.extend(self.FileSize.to_bytes(8, 'little'))
        Bytes.extend(self.FileOffset.to_bytes(8, 'little'))
        return Bytes

#FileRecordArray = []

#Offset = 4 + 4000 * (1024 + 8 + 8)

#for FilePath in FileList:
#    fileRecord = FileRecord()
#    fileRecord.FileName = FilePath[FilePath.rfind('\\') + 1:-7]
#    fileRecord.FileSize = os.stat(FilePath).st_size
#    fileRecord.FileOffset = Offset
#    Offset = Offset + fileRecord.FileSize
#    FileRecordArray.append(fileRecord)
  
#os.chdir("F:/Paradox/Build/GameContent/AssetPackages")

#f = open("Objects.assetpackage", "wb")

#f.write(int(4000).to_bytes(4, 'little'))

#for fileRecord in FileRecordArray:
#    f.write(fileRecord.Serialize())

#os.chdir("F:/Paradox/Build/GameContent/Objects")

#for FilePath in FileList:
#    f1 = open(FilePath[FilePath.rfind('\\') + 1:], "rb")
#    f.write(f1.read())

#f.close()

def PackFilesIntoArchive(FilesFolder, ArchiveName):
    FileList = glob.glob(FilesFolder + "/*.*")
    FileRecordArray = []

    Offset = 4 + len(FileList) * (1024 + 8 + 8)

    for FilePath in FileList:
        fileRecord = FileRecord()
        fileRecord.FileName = FilePath[FilePath.rfind('\\') + 1:FilePath.rfind('.')]
        fileRecord.FileSize = os.stat(FilePath).st_size
        fileRecord.FileOffset = Offset
        Offset = Offset + fileRecord.FileSize
        FileRecordArray.append(fileRecord)

    os.chdir("F:/Paradox/Build/GameContent/AssetPackages")

    f = open(ArchiveName + ".assetpackage", "wb")

    f.write(int(len(FileList)).to_bytes(4, 'little'))

    for fileRecord in FileRecordArray:
        f.write(fileRecord.Serialize())

    os.chdir(FilesFolder)

    for FilePath in FileList:
        f1 = open(FilePath[FilePath.rfind('\\') + 1:], "rb")
        f.write(f1.read())

    f.close()

PackFilesIntoArchive("F:/Paradox/Build/GameContent/Objects", "Objects")
PackFilesIntoArchive("F:/Paradox/Build/GameContent/Textures", "Textures")
PackFilesIntoArchive("F:/Paradox/Build/GameContent/Shaders", "Shaders")