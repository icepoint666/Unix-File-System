# Unix-File-System

# Setup
```shell
$ g++ ufs.cpp -o ufs

$ ./ufs
```

# Command
- Create a file: `createFile fileName fileSize`
```shell
createFile dir1/myFile 10
```
- Delete a file: `deleteFile fileName`
```shell
deleteFile dir1/myFile
```
- Create a directory: `createDir dirName`
```shell
createDir dir2
```
- Delete a directory: `deleteDir dirName`
```shell
deleteDir dir2
```
- Change current working directory: `changeDir dirName`
```shell
changeDir dir2
```
- List all the files and directories: `dir`
```shell
dir
```
- Copy a file: `cp`
```shell
cp file1 file2
```

- Display the usage of storage space: `sum`
```shell
sum

SUM BLOCKS: 16384
SUPER BLOCKS: 20
INODE BLOCKS: 364
USE BLOCKS / FREE BLOCKS: 49 / 16000
```
- Print out the file contents: `cat`
```shell
cat
```
