import os
import glob
import sys

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

def PackFilesIntoArchive(FilesFolder, ArchiveName):
    FileList = glob.glob(FilesFolder + "/**/*.*", recursive = True)
    FileRecordArray = []

    Offset = 4 + len(FileList) * (1024 + 8 + 8)

    for FilePath in FileList:
        fileRecord = FileRecord()
        fileRecord.FileName = FilePath[FilePath.find('\\') + 1:FilePath.rfind('.')]
        fileRecord.FileName = fileRecord.FileName.replace('\\', '.')
        fileRecord.FileSize = os.stat(FilePath).st_size
        fileRecord.FileOffset = Offset
        Offset = Offset + fileRecord.FileSize
        FileRecordArray.append(fileRecord)

    os.chdir("F:/Paradox/Build/AssetPackages")

    f = open(ArchiveName + ".assetpackage", "wb")

    f.write(int(len(FileList)).to_bytes(4, 'little'))

    for fileRecord in FileRecordArray:
        f.write(fileRecord.Serialize())

    os.chdir(FilesFolder)

    for FilePath in FileList:
        f1 = open(FilePath[FilePath.find('\\') + 1:], "rb")
        f.write(f1.read())

    f.close()

if __name__ == "__main__":
    PackFilesIntoArchive(sys.argv[1], sys.argv[2])