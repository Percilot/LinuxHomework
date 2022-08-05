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
![Screenshot from 2022-07-23 23-26-11](/home/l123456/Pictures/Screenshot from 2022-07-23 23-26-11.png)

![Screenshot from 2022-07-23 23-27-14](/home/l123456/Pictures/Screenshot from 2022-07-23 23-27-14.png)

![Screenshot from 2022-07-23 23-27-40](/home/l123456/Pictures/Screenshot from 2022-07-23 23-27-40.png)

![Screenshot from 2022-07-23 23-29-05](/home/l123456/Pictures/Screenshot from 2022-07-23 23-29-05.png)

![Screenshot from 2022-07-23 23-29-43](/home/l123456/Pictures/Screenshot from 2022-07-23 23-29-43.png)

![Screenshot from 2022-07-23 23-30-11](/home/l123456/Pictures/Screenshot from 2022-07-23 23-30-11.png)

##### 2.1

在本题中，我们直接使用`gcc`命令，生成`power`可执行文件：

```shell
gcc -c main.c input.c compute.c
gcc main.o input.o compute.o -o power -lm
```

![Screenshot from 2022-07-23 23-32-50](/home/l123456/Pictures/Screenshot from 2022-07-23 23-32-50.png)

尝试运行可执行文件`power`，可以得到如下的运行结果：

```shell
./power
```

![Screenshot from 2022-07-23 23-33-51](/home/l123456/Pictures/Screenshot from 2022-07-23 23-33-51.png)

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

![Screenshot from 2022-07-23 23-52-12](/home/l123456/Pictures/Screenshot from 2022-07-23 23-52-12.png)

接下来我们执行`make`命令，编译出可执行文件`power`：

![Screenshot from 2022-07-23 23-53-01](/home/l123456/Pictures/Screenshot from 2022-07-23 23-53-01.png)

执行`power`，可以得到如下的运行结果：

![Screenshot from 2022-07-23 23-53-43](/home/l123456/Pictures/Screenshot from 2022-07-23 23-53-43.png) 

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

![Screenshot from 2022-07-25 20-19-22](/home/l123456/Pictures/Screenshot from 2022-07-25 20-19-22.png)

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

![Screenshot from 2022-07-25 21-18-51](/home/l123456/Pictures/Screenshot from 2022-07-25 21-18-51.png)

可以看到，我们编写的脚本成功实现了目标。

+++