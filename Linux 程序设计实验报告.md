## Linux 程序设计实验报告

### 实验环境

- OS：64位 Ubuntu 20.04.4 LTS

### 实验内容

#### 1 在操作系统课程实验中，要用make工具编译内核，要掌握make和makefile。makfile文件中的每一行是描述文件间依赖关系的make规则。本实验是关于makefile内容的，您不需要在计算机上进行编程运行，只要书面回答下面这些问题。

#### 对于下面的makefile:

```makefile
CC = gcc
OPTIONS = -O3 -o
OBJECTS = main.o stack.o misc.o
SOURCES = main.c stack.c misc.c
HEADERS = main.h stack.h misc.h
polish: main.c $(OBJECTS)
$(CC) $(OPTIONS) power $(OBJECTS) -lm
main.o: main.c main.h misc.h
stack.o: stack.c stack.h misc.h
misc.o: misc.c misc.h
```

#### 回答下列问题

#### a. 所有宏定义的名字？

#### b. 所有目标文件的名字？

#### c. 每个目标的依赖文件？

#### d. 生成每个目标文件所需执行的命令？

#### e. 画出makefile对应的依赖关系树？

#### f. 生成main.o stack.o和misc.o时会执行哪些命令，为什么？

+++

#### a. 所有宏定义的名字

我们将所有宏定义列举如下：

`CC、OPTIONS、OBJECTS、SOURCES、HEADERS`

#### b. 所有目标文件的名字

我们将所有目标文件列举如下：

`polish、main.o、stack.o、misc.o`

#### c. 每个目标的依赖文件

对于`polish`，依赖文件如下：`main.c、main.o、stack.o、misc.o`

对于`main.o`，依赖文件如下：`main.c、main.h、misc.h`

对于`stack.o`，依赖文件如下：`stack.c、stack.h、misc.h`

对于`misc.o`，依赖文件如下：`misc.c、misc.h`

#### d. 生成每个目标文件所需执行的命令

对于`polish`，命令如下：`gcc -O3 -o power main.o stack.o misc.o -lm`

对于`main.o`，命令如下：`gcc -c main.c -o main.o`

对于`stack.o`，命令如下：`gcc -c stack.c -o stack.o`

对于`misc.o`，命令如下：`gcc -c misc.c -o misc.o`

#### e. 画出makefile对应的依赖关系树



#### f. 生成main.o stack.o和misc.o时会执行哪些命令

事实上，这三个`.o`文件都是由单个`.c`文件编译生成，因此会执行如下的指令：

生成`main.o`：`gcc -c main.c -o main.o`

生成`stack.o`：`gcc -c stack.c -o stack.o`

生成`misc.o`：`gcc -c misc.c -o misc.o`

+++

#### 2 用编辑器创建main.c、compute.c、input.c、compute.h、input.h和main.h文件。下面是它们的内容。注意compute.h和input.h文件仅包含了compute和input函数的声明但没有定义。定义部分是在compute.c和input.c文件中。

我们首先根据题目要求，创建对应文件，并写入相应的代码：
![2.2directory](/home/l123456/Pictures/2.2directory1.png)

##### 2.1

在本题中，我们直接使用`gcc`命令，生成`power`可执行文件：

```shell
gcc -c main.c input.c compute.c
gcc main.o input.o compute.o -o power -lm
```

尝试运行可执行文件`power`，可以得到如下的运行结果：

```shell
./power
```

![2.2result1](/home/l123456/Pictures/2.2result1.png)

##### 2.2

仿照第一题，我们编写如下的`makefile`文件：

```makefile
CC = gcc
HEADERS = main.h input.h compute.h
SOURCES = main.c input.c compute.c
OBJECTS = main.o input.o compute.o
power: $(OBJECTS)
	$(CC) $(OBJECTS) -o power -lm
main.o: main.c $(HEADERS)
	$(CC) -c main.c -o main.o
input.o: input.c input.h
	$(CC) -c input.c -o input.o
compute.o: compute.c compute.h
	$(CC) -c compute.c -o compute.o
```

![2.2directory2](/home/l123456/Pictures/2.2directory2.png)

接下来我们执行`make`命令，编译出可执行文件`power`。

执行`power`，可以得到如下的运行结果：

![Screenshot from 2022-08-05 18-45-39](/home/l123456/Pictures/2.2result2.png)

+++

#### 3 编写shell脚本，统计指定目录下的普通文件、子目录及可执行文件的数目，统计该目录下所有普通文件字节数总和，目录的路径名字由参数传入。（不能使用sed、awk等工具）

我们首先给出实现的代码：

```shell
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
```

接下来，我们尝试运行此脚本，并给出运行结果：

![Screenshot from 2022-08-05 18-48-23](/home/l123456/Pictures/Screenshot from 2022-08-05 18-48-23.png)

可以看到，我们编写的脚本成功实现了目标。

+++

#### 4 编写shell 脚本，输入一个字符串，忽略（删除）非字母后，检测该字符串是否为回文(palindrome)。对于一个字符串，如果从前向后读和从后向前读都是同一个字符串，则称之为回文串。例如，单词“mom”，“dad”和“noon”都是回文串。（不能使用sed、awk、tr、rev等工具）

我们首先给出实现的代码：

```shell
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
```

接下来，我们尝试运行此脚本，并给出运行结果：

![Screenshot from 2022-08-05 18-50-57](/home/l123456/Pictures/Screenshot from 2022-08-05 18-50-57.png)

可以看到，我们编写的脚本成功实现了目标。

+++

#### 5 编写一个实现文件备份和同步的shell脚本程序dirsync。程序的参数是两个需要备份同步的目录，如:dirsync ~\dir1 ~\dir2  \# ~\dir1为源目录，~\dir2为目标目录dirsync程序实现两个目录内的所有文件和子目录（递归所有的子目录）内容保持一致。

#### 程序基本功能如下。

#### 1)备份功能：目标目录将使用来自源目录的最新文件，新文件和新子目录进行升级，源目录将保持不变。dirsync程序能够实现增量备份。

#### 2)同步功能：两个方向上的旧文件都将被最新文件替换，新文件都将被双向复制。源目录被删除的文件和子目录，目标目录也要对应删除。

#### 3)其它功能自行添加设计。

我们首先给出实现的代码：

```shell
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
```

接下来，我们尝试运行此脚本，并给出运行结果：

1. 对备份功能的测试：

   调用功能之前，源目录如下：

   ![Screenshot from 2022-08-05 23-10-50](/home/l123456/Pictures/Screenshot from 2022-08-05 23-10-50.png)

   目标目录如下：（不存在）

   执行相应功能：

   ![Screenshot from 2022-08-05 23-14-28](/home/l123456/Pictures/Screenshot from 2022-08-05 23-14-28.png)

   可以看到，我们编写的脚本成功备份了源目录。

2. 对同步功能测试如下

   我们向`test1`下的`test`文件和`test2/twst`下的`te15`写入一些数据，运行前的状态如下：

   `./test1/test`

   ![Screenshot from 2022-08-05 23-18-25](/home/l123456/Pictures/Screenshot from 2022-08-05 23-18-25.png)

   `./test2/test`

   ![Screenshot from 2022-08-05 23-19-27](/home/l123456/Pictures/Screenshot from 2022-08-05 23-19-27.png)

   `./test1/twst0/te15`

   ![Screenshot from 2022-08-05 23-20-55](/home/l123456/Pictures/Screenshot from 2022-08-05 23-20-55.png)

   `./test2/twst0/te15`

   ![Screenshot from 2022-08-05 23-22-39](/home/l123456/Pictures/Screenshot from 2022-08-05 23-22-39.png)

   运行后的状态如下：

   ![Screenshot from 2022-08-05 23-24-11](/home/l123456/Pictures/Screenshot from 2022-08-05 23-24-11.png)

   再次查看进行过对应的文件：

   `./test1/test`

   ![Screenshot from 2022-08-05 23-28-36](/home/l123456/Pictures/Screenshot from 2022-08-05 23-28-36.png)

   `./test2/test`

   ![Screenshot from 2022-08-05 23-29-16](/home/l123456/Pictures/Screenshot from 2022-08-05 23-29-16.png)

   `./test1/twst0/te15`

   ![Screenshot from 2022-08-05 23-29-51](/home/l123456/Pictures/Screenshot from 2022-08-05 23-29-51.png)

   `./test2/twst0/te15`

   ![Screenshot from 2022-08-05 23-31-23](/home/l123456/Pictures/Screenshot from 2022-08-05 23-31-23.png)

可以看到，我们编写的脚本成功实现两个目录间的同步。

综上所述，我们编写的脚本成功实现了对应的功能。

+++