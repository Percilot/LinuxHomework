#!/bin/bash

# 全局变量定义，用于统计各项数据
# 目录总数
DirectoryCount=0
# 可执行文件总数
ExecutableFileCount=0
# 普通文件总数
NormalFileCount=0
# 普通文件字节总数
ByteCount=0

# 递归函数，统计各项数据
function CountFile()
{
    # 指定目录
    Position=$1"/*"
    # 遍历目录中所有文件
    for file in $Position
        do
            # 对于目录文件的处理
            if [ -d "$file" ]
            then
                # 目录文件总数加一
                DirectoryCount=`expr $DirectoryCount + 1`
                # 递归调用，统计子目录下数据
                CountFile "$file"
            # 对于可执行文件的处理
            elif [ -x "$file" ]
            then
                # 可执行文件总数加一
                ExecutableFileCount=`expr $ExecutableFileCount + 1`
            # 对于普通文件的处理
            elif [ -f "$file" ]
            then
                # 调用set指令，统计普通文件字节总数
                set - `ls -l $file`
                # ls -l指令第五个结果即为字节总数，故普通字节总数需要加上set指令的第五个结果
                ByteCount=`expr $ByteCount + $5`
                # 普通文件总数加一
                NormalFileCount=`expr $NormalFileCount + 1`
            fi
        done
}

# 总函数部分
# 传入参数不为一，证明传入参数个数错误
if [ $# -ne 1 ]
 then
    echo "参数个数错误，请检查您传入的参数个数"
    exit 1
fi

# 传入的参数为合法的路径文件
if [ -d $1 ]
 then
    # 调用递归函数统计数据
    CountFile $1
    echo "目标目录下共有："
    # 打印各项统计结果
    echo "$DirectoryCount 个目录文件"
    echo "$ExecutableFileCount 个可执行文件"
    echo "$NormalFileCount 个普通文件"
    echo "普通文件共 $ByteCount B"
# 传入的参数为非法路径
else
    echo "传入参数为非法路径！"
    exit 1
fi