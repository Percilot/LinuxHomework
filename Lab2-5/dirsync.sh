#!/bin/bash

# 本函数主要负责实现备份功能
function Copy()
{
    # 备份功能比较容易实现
    # 具体操作分为两步
    SrcPos=$1"/*"
    DstPos=$2"/*"
    # 第一步，删除目标目录下所有文件
    rm -rf $DstPos
    # 第二步，将源目录下所有文件复制到目标目录下
    cp -rfp $SrcPos $2
}

# 第一遍扫描，扫描目标目录
function ScanDst()
{
    # 获取目标目录下所有文件
    DstPos=$2"/*"
    # 对于目标目录下每一个文件进行处理
    for FileInDst in $DstPos
        do
            # 根据目标目录下文件的路径对应生成源目录对应文件的路径
            FileInSrc="${FileInDst/#$2/$1}"
            if [[ -d "$FileInDst" ]]
            then
                # 目标目录下文件是目录文件
                if [[ -d "$FileInSrc" ]]
                then
                    # 如果源目录下存在对应的文件
                    # 递归调用，处理子目录
                    ScanDst "$FileInSrc" "$FileInDst"
                else
                    # 如果源目录下不存在对应的文件
                    # 这表明这一文件不应该存在，删除此文件
                    rm -rf $FileInDst
                fi
            else
                # 目标目录下文件不是目录文件
                if [[ !(-e "$FileInSrc") || (-d "$FileInSrc") ]]
                then
                    # 若源目录下不存在对应的文件或者源目录下对应的文件类型不匹配
                    # 这表明这一文件不应该存在，删除此文件
                    rm -rf $FileInDst
                elif [[ $FileInDst -ot $FileInSrc ]]
                then
                    # 源目录下对应的文件存在且类型匹配，则在目标目录下保留最晚更新的文件
                    cp -fp $FileInSrc $FileInDst
                fi
            fi
        done
}

# 第二遍扫描，扫描源目录
function ScanSrc()
{
    # 获取源目录下所有文件
    SrcPos=$1"/*"
    # 对于源目录下每一个文件进行处理
    for FileInSrc in $SrcPos
        do
            # 根据源目录下文件的路径对应生成目标目录对应文件的路径
            FileInDst="${FileInSrc/#$1/$2}"
            # 源目录下文件是目录文件
            if [[ -d "$FileInSrc" ]]
            then
                if [[ -d "$FileInDst" ]]
                then
                    # 如果目标目录下存在对应的文件
                    # 递归调用，处理子目录
                    ScanSrc "$FileInSrc" "$FileInDst"
                else
                    # 如果目标目录下不存在对应的文件
                    # 这表明这一文件应该被复制到目标目录中，复制此文件
                    cp -rfp $FileInSrc $FileInDst
                fi
            else
                # 目标目录下文件不是目录文件
                if [[ !(-e "$FileInDst") || (-d "$FileInDst") ]]
                then
                    # 若目录目录下不存在对应的文件或者目标目录下对应的文件类型不匹配
                    # 这表明这一文件应该覆盖目标目录下对应文件，复制此文件
                    cp -fp $FileInSrc $FileInDst
                elif [[ $FileInSrc -ot $FileInDst ]]
                then
                    # 目标目录下对应的文件存在且类型匹配，则在源目录下保留最晚更新的文件
                    cp -fp $FileInDst $FileInSrc
                fi
            fi
        done
}

# 此部分为提示部分
echo "0号功能为备份功能"
echo "1号功能为同步功能"
# 读入用户选择的功能
read -p "请输入需要使用的功能：" Mode

# 检查传入的参数
# 参数个数不为2，认定为非法输入
# 第一个参数不是目录文件，认定为非法输入
if [[ $# -ne 2 || !(-d $1) ]]
then
    echo "参数误，请检查您传入的参数！"
    exit 1
else
    # 若第二个参数对应的目录文件不存在，则新建对应目录
    if [[ !(-e $2) ]]
    then
        mkdir $2
    fi
    # 选用备份功能
    if [[ $Mode == 0 ]]
    then
        # 调用copy函数实现复制
        Copy $1 $2
        exit 0
    # 选用同步功能
    elif [[ $Mode == 1 ]]
    then
        # 通过两遍扫描实现同步功能
        # 具体实现见函数部分的描述
        ScanDst $1 $2
        ScanSrc $1 $2
        exit 0
    # 非法选择
    else
        echo "未知选择！"
        exit 1
    fi
fi