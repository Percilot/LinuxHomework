#!/bin/bash

# 输出提示语句，并读入用户想要检查的字符串
read -p "请输入想要检查的字符串：" Input
# 获取输入字符串的长度，为之后的筛选做准备
InputLength=${#Input}

# 遍历字符串中每一个字符，去掉所有非字母字符
for((i=0; i<$InputLength; i++))
do
    # 获取对应单个字符
    Char=${Input:i:1}
    # 检查字符是否是字母
    if [[ $Char =~ [a-Z] ]]
    then
        # 是字母，将字母保存入待检测的字符串中
        DetectWord=${DetectWord}${Char}
    fi
done

# Debug时使用，打印去除非字母后的字符串
# echo "$DetectWord"

# 获取待检测字符串的长度，为之后的比较做准备
DetectWordLength=${#DetectWord}

# 分别遍历前一半和后一半字符串，比较对应位置字母是否相同
for((i=0; i<`expr $DetectWordLength / 2`; i++))
do
    # Char1对应前一半字符
    Char1=${DetectWord:i:1}
    # Char2对应后一半字符
    Char2=${DetectWord:`expr $DetectWordLength - 1 - $i`:1}
    # 比较Char1和Char2
    if [ $Char1 != $Char2 ]
    then
        # 两者不相同，输出提示信息并退出程序
        echo "此字符串不是回文字符串"
        exit 0
    fi
done

# 所有比较均通过，输出提示信息并退出程序
echo "此字符串是回文字符串"
exit 0