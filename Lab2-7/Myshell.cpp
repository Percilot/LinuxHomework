/* 头文件 */
#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <cstring>
#include <fstream>

using namespace std;

/*--------------------------------------------------------------------------*/

/* 宏定义 */

// 最大同时执行工作数量
#define MAX_WORK_COUNT 1024
// 字符串最大允许长度
#define MAX_STRING_SIZE 1024
// 最大参数个数
#define MAX_PARA_COUNT 1024
// 判定是否是八进制数字
#define IS_OCT_DIGIT(SRC) (SRC >= '0' && SRC <= '7')

/*--------------------------------------------------------------------------*/

/* 全局变量定义区 */

// 用于存储写入终端的信息
char Buffer[MAX_STRING_SIZE];
// 当前执行的指令
string NowCommand;
// 判定输入是否来自终端
bool IsInTerminal;
// 判定输出是否来自终端
bool IsOutTerminal;
// 终端输入来源
int TerminalIn;
// 终端输出去向
int TerminalOut;

// 当前指令是否能够执行状态（能否正常执行）
int State;

// 主机名
string HostName;
// 用户名
string UserName;
// 用户主目录
string HomeDir;
// 本程序所在路径
string ShellDir;
// 帮助文档所在路径
string HelpPath;
// 当前路径
string CurrentPath;
// 输出到标准输出的信息
string InfoToStdOutput;
// 输出到错误的信息
string InfoToStdError;

// 当前进程进程号
int PID;
// 子进程的进程号
int SonPID;
// 参数个数
int _Argc;
// 参数内容
string _Argv[MAX_PARA_COUNT];
// 子进程进程号数组
int Jobs[MAX_WORK_COUNT];
// 子进程状态
// 0表示正常执行，1表示后台运行，2表示被挂起
int States[MAX_WORK_COUNT];
// 子进程执行的命令
string CMDofSons[MAX_WORK_COUNT];
// 记录第一个子进程对应数组下标
int Head;
// 记录最后一个子进程对应数组下标
int Tail;

/*--------------------------------------------------------------------------*/

/* 辅助函数定义区 */
/* 这些函数是用于辅助其他函数执行的 */

// ReturnJobsInfo函数负责将传入的下标对应的子进程相关信息拼接为一个字符串
string ReturnJobsInfo(int JobIndex, int Finished = 0);
// ReturnUmaskValue函数负责将传入的Mode转换为输出信息
string ReturnUmaskValue(mode_t CurrentMode);
// GetOctValue函数负责将传入的八进制数字符串转换为对应的数值
int GetOctValue(string Src);
// TimeCommpare函数负责比较两个时间类型的变量
int TimeCommpare(const timespec &Time1, const timespec &Time2);
// UpdateJobs函数负责更新子进程数组
void UpdateJobs();

/*--------------------------------------------------------------------------*/

/* 主体函数定义区 */
/* 这些函数是程序的主体函数，它们负责实现程序的功能 */

// SignalDeal函数负责处理Ctrl+C等组合键，即处理外部信号
void SignalDeal(int Signal);

// InitShell函数负责初始化
void InitShell(int __Argc, char **__Argv);

// AnalyseCommand函数负责分流前台运行指令和后台运行指令
void AnalyseCommand(string Command);

// DealPipeCommand函数负责分流含管道指令和不含管道的指令
void DealPipeCommand(string DividedCommand[], int ParaCount, bool WithFork = true);

// ExecCommand函数负责处理输入输出和具体执行指令
void ExecCommand(string DividedCommand[], int ParaCount, bool WithFork = true);

// 处理Cd指令
void ExecCommandCd(string DividedCommand[], int ParaCount);

// 处理Exit指令
void ExecCommandExit(string DividedCommand[], int ParaCount);

// 处理Clr指令
void ExecCommandClr(string DividedCommand[], int ParaCount);

// 处理Time指令
void ExecCommandTime(string DividedCommand[], int ParaCount);

// 处理Pwd指令
void ExecCommandPwd(string DividedCommand[], int ParaCount);

// 处理Echo指令
void ExecCommandEcho(string DividedCommand[], int ParaCount);

// 处理Dir指令
void ExecCommandDir(string DividedCommand[], int ParaCount);

// 处理Test指令
void ExecCommandTest(string DividedCommand[], int ParaCount);

// 处理Jobs指令
void ExecCommandJobs(string DividedCommand[], int ParaCount);

// 处理Bg指令
void ExecCommandBg(string DividedCommand[], int ParaCount);

// 处理Fg指令
void ExecCommandFg(string DividedCommand[], int ParaCount);

// 处理Set指令
void ExecCommandSet(string DividedCommand[], int ParaCount);

// 处理Umask指令
void ExecCommandUmask(string DividedCommand[], int ParaCount);

// 处理Exec指令
void ExecCommandExec(string DividedCommand[], int ParaCount);

// 处理Help指令
void ExecCommandHelp(string DividedCommand[], int ParaCount);

/*--------------------------------------------------------------------------*/

/* 主函数部分 */
int main(int Argc, char *Argv[])
{
    // 根据传入的参数，初始化环境
    InitShell(Argc, Argv);
    // 通过循环实现不断读入指令并执行
    int Flag = 1;
    while (Flag)
    {
        // 若输入来自终端
        if (IsInTerminal)
        {
            // 仿照Shell的风格，向终端中输出用户名等相关信息
            sprintf(Buffer, "\e[1;32m%s@%s\e[0m:\e[1;34m%s\e[0m$ ", UserName.c_str(), HostName.c_str(), CurrentPath.c_str());
            // 复制一份写入输出中
            string Temp = Buffer;
            write(TerminalOut, Temp.c_str(), Temp.length());
        }

        // 读入指令
        int i;
        for (i = 0;; ++i)
        {
            if (read(STDIN_FILENO, Buffer + i, 1) <= 0)
            {
                Flag = 0;
                break;
            }
            if (Buffer[i] == '\n')
                break;
        }
        // 在数组末尾加入终止符
        Buffer[i] = '\0';
        // 开始执行指令
        AnalyseCommand(Buffer);
    }
    return 0;
}

/*--------------------------------------------------------------------------*/

/* 辅助函数实现区 */
/* 这里是辅助函数的具体实现 */

// ReturnJobsInfo函数负责将传入的下标对应的子进程相关信息拼接为一个字符串
string ReturnJobsInfo(int JobIndex, int Finished)
{
    string Result = "";
    if (!(JobIndex < Head || JobIndex >= Tail))
    {
        // 对于合法的任务号的处理
        // 输出任务号和对应的子进程进程号
        Result = "[" + to_string(JobIndex + 1) + "]\t" + to_string(Jobs[JobIndex]) + "\t";
        // 输出任务状态
        if (Finished)
            Result += "Finished";
        else if (States[JobIndex] == 1)
            Result += "Running";
        else
            Result += "Hanging";
        // 输出任务内容
        Result += "\t";
        Result += CMDofSons[JobIndex];
        Result += "\n";
    }
    return Result;
}

// ReturnUmaskValue函数负责将传入的Mode转换为输出信息
string ReturnUmaskValue(mode_t CurrentMode)
{
    string Result = "";
    // 分别获取八进制下umask各位数字并拼接为字符串
    Result += to_string((CurrentMode >> 9) & 7);
    Result += to_string((CurrentMode >> 6) & 7);
    Result += to_string((CurrentMode >> 3) & 7);
    Result += to_string(CurrentMode & 7);
    Result += "\n";
    return Result;
}

// GetOctValue函数负责将传入的八进制数字符串转换为对应的数值
int GetOctValue(string Src)
{
    int Result = 0;
    for (int i = 0; i < Src.length(); ++i)
    {
        // 通过宏定义判定对应字符是否是合法的八进制数字
        if (!IS_OCT_DIGIT(Src[i]))
        {
            // 非法字符串
            Result = -1;
            break;
        }
        // 正常处理
        Result = Result * 8 + (Src[i] - '0');
    }
    return Result;
}

// TimeCommpare函数负责比较两个时间类型的变量
int TimeCommpare(const timespec &Time1, const timespec &Time2)
{
    // 分别比较结构体中两个元素即可
    if (Time1.tv_sec > Time2.tv_sec)
        return 1;
    else if (Time1.tv_sec < Time2.tv_sec)
        return -1;
    else if (Time1.tv_nsec > Time2.tv_nsec)
        return 1;
    else if (Time1.tv_nsec < Time2.tv_nsec)
        return -1;
    else
        return 0;
}

// UpdateJobs函数负责更新子进程数组
void UpdateJobs()
{
    // 扫描任务表
    for (int i = Head; i < Tail; ++i)
    {
        if (States[i] && Jobs[i] == waitpid(Jobs[i], NULL, WNOHANG))
        {
            if (IsInTerminal)
            {
                // 输出已经的完成进程
                string Temp = ReturnJobsInfo(i, 1);
                write(TerminalOut, Temp.c_str(), Temp.length());
            }
            // 更新任务表
            States[i] = 0;
            if (Head == i)
                ++Head;
        }
    }
    if (Head == Tail)
        Head = Tail = 0;
    return;
}

/*--------------------------------------------------------------------------*/

/* 主体函数实现区 */
/* 这里是主体函数的具体实现 */

// SignalDeal函数负责处理Ctrl+C等组合键，即处理外部信号
void SignalDeal(int Signal)
{
    switch (Signal)
    {
    // Ctrl+C，终止当前进程
    case SIGINT:
        write(TerminalOut, "\n", 1);
        break;
    // Ctrl+Z，挂起当前进程
    case SIGTSTP:
        write(TerminalOut, "\n", 1);
        if (SonPID != -1)
        {
            // 挂起进程
            setpgid(SonPID, 0);
            // 发送信号
            kill(SonPID, SIGTSTP);
            // 更新任务表、状态表和对应的命令
            Jobs[Tail] = SonPID;
            States[Tail] = 2;
            CMDofSons[Tail] = NowCommand;

            if (IsInTerminal)
            {
                // 打印子进程表
                string Result = ReturnJobsInfo(Tail);
                write(TerminalOut, Result.c_str(), Result.length());
            }
            Tail++;
            SonPID = -1;
        }
        break;
    default:
        break;
    }
}

// InitShell函数负责初始化
void InitShell(int __Argc, char **__Argv)
{
    // 设置全局变量
    char Buffer[MAX_STRING_SIZE] = {0};
    _Argc = __Argc;
    for (int i = 0; i < __Argc; ++i)
        _Argv[i] = __Argv[i];

    // 超过两个参数，则将标准输入重定向
    int InputSrc = -1;
    if (__Argc > 1)
    {
        // 输入重定向
        InputSrc = open(_Argv[1].c_str(), O_RDONLY);
        if (InputSrc < 0)
        {
            sprintf(Buffer, "ERROR: Can't open target file: %s !\n", _Argv[1].c_str());
            write(STDERR_FILENO, Buffer, MAX_STRING_SIZE);
            exit(0);
        }
        dup2(InputSrc, STDIN_FILENO);
        close(InputSrc);
    }
    // 获取进程号
    PID = getpid();
    // 初始化子进程号
    SonPID = -1;

    // 获取主机名
    gethostname(Buffer, MAX_STRING_SIZE);
    HostName = Buffer;
    // 获取用户名
    UserName = getenv("USERNAME");
    // 获取主目录路径
    HomeDir = getenv("HOME");
    // 获取当前路径
    CurrentPath = getenv("PWD");

    HelpPath = CurrentPath + "/Help";

    // 获取程序自身的路径
    int Length = readlink("/proc/self/exe", Buffer, MAX_STRING_SIZE);
    Buffer[Length] = '\0';
    // 以程序自身路径设置环境变量
    ShellDir = Buffer;
    setenv("SHELL", Buffer, 1);
    // 设置父进程路径为bash
    setenv("PARENT", "\\bin\\bash", 1);

    // 初始化子进程表
    Head = Tail = 0;
    // 记录终端标准输入输出
    TerminalIn = open("/dev/tty", O_RDONLY);
    TerminalOut = open("/dev/tty", O_WRONLY);

    // 判断标准输入输出是否是来自终端
    struct stat FileInfo;
    fstat(STDIN_FILENO, &FileInfo);
    IsInTerminal = S_ISCHR(FileInfo.st_mode);
    fstat(STDOUT_FILENO, &FileInfo);
    IsOutTerminal = S_ISCHR(FileInfo.st_mode);

    if (IsInTerminal)
    {
        // 在终端中，设置信号处理函数
        signal(SIGINT, SignalDeal);
        signal(SIGTSTP, SignalDeal);
    }
    return;
}

// AnalyseCommand函数负责分流前台运行指令和后台运行指令
void AnalyseCommand(string Command)
{
    // 更新子进程表
    UpdateJobs();

    // 通过流将指令分割
    stringstream TempStream;
    // 指令被分割成的段数
    int ParaCount = 0;
    // 被分割后的指令
    string DividedCommand[MAX_PARA_COUNT];
    TempStream << Command;

    // 通过循环，以空格为分界线，将指令分割并存储
    while (true)
    {
        DividedCommand[ParaCount] = "";
        TempStream >> DividedCommand[ParaCount];
        if (DividedCommand[ParaCount] == "")
            break;
        ++ParaCount;
    }

    // 对非空后台运行指令的处理
    if (ParaCount > 0 && DividedCommand[ParaCount - 1] == "&")
    {
        // fork出子进程
        int pid = fork();
        if (pid)
        {
            // 父进程中，存储子进程相关信息
            Jobs[Tail] = pid;
            States[Tail] = 1;
            CMDofSons[Tail] = "";
            for (int i = 0; i < ParaCount - 1; ++i)
                CMDofSons[Tail] += (DividedCommand[i] + " ");
            NowCommand = CMDofSons[Tail];

            // 如果输入来自终端，打印子进程表
            if (IsInTerminal)
            {
                string Temp = ReturnJobsInfo(Tail);
                write(TerminalOut, Temp.c_str(), Temp.length());
            }
            ++Tail;
        }
        else
        {
            // 子进程中，调用DealPipeCommand执行指令
            setpgid(0, 0);
            // 传入(Paracount - 1)是为了去除末尾的"&"
            DealPipeCommand(DividedCommand, ParaCount - 1, false);
            // 执行完毕，退出子进程
            exit(0);
        }
    }
    // 对非空前台指令的处理
    else
    {
        // 更新当前运行的指令
        NowCommand = Command;
        // 调用DealPipeCommand执行指令
        DealPipeCommand(DividedCommand, ParaCount);
    }
    return;
}

// DealPipeCommand函数负责分流含管道指令和不含管道的指令
void DealPipeCommand(string DividedCommand[], int ParaCount, bool WithFork)
{
    // 检查需要执行的指令中是否含有管道
    bool PipeExistsFlag = false;
    for (int i = 0; i < ParaCount; ++i)
    {
        if (DividedCommand[i] == "|")
        {
            PipeExistsFlag = true;
            break;
        }
    }
    //  不存在管道符，直接调用ExecCommand执行指令
    if (PipeExistsFlag == false)
    {
        ExecCommand(DividedCommand, ParaCount);
        return;
    }

    // 存在管道符
    // 通过fork生成子进程，并将其通过管道连接
    if (WithFork)
        SonPID = fork();

    // 父进程
    if (WithFork && SonPID)
    {
        // 等待子进程完成任务
        while (SonPID != -1 && !waitpid(SonPID, NULL, WNOHANG))
            ;
        SonPID = -1;
    }
    // 子进程
    else
    {
        // 将信号处理函数恢复为系统默认处理函数
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        // 记录上一个管道符位置
        int LastPipe = -1;

        // 通过两个管道实现管道符对于两条不同指令的连接
        int FirstFD[2], SecondFD[2];

        // 存储所有子进程进程号，便于判断所有任务是否完成
        int PipeBuffer[MAX_PARA_COUNT];

        // 管道总数
        int PipeCount = 0;

        // 在指令末尾添加一个管道符，用于标记结束
        DividedCommand[ParaCount++] = "|";

        // 循环，为每一个管道符创建管道
        for (int i = 0; i < ParaCount; ++i)
        {
            if (DividedCommand[i] == "|")
            {
                // 遇到的第一个管道符
                if (LastPipe == -1)
                {
                    // 将第一个管道的读端设定为标准输入
                    // 这样使得指令总体可以从标准输入中读取信息
                    FirstFD[0] = STDIN_FILENO;
                    // 将第一个管道的写端设定为非法
                    // 这是由于我们在末尾添加了一个管道，所以这里可以暂时这样安排
                    FirstFD[1] = -1;
                    // 创建后一个管道
                    pipe(SecondFD);
                }
                else if (i == ParaCount - 1)
                {
                    // 遇到的最后一个管道符，其实是我们之前添加的
                    // 关闭前一个管道的读端，这里是为了防止误关标准输入
                    if (FirstFD[0] != STDIN_FILENO)
                        close(FirstFD[0]);

                    // 更新前一个管道的信息
                    FirstFD[0] = SecondFD[0];
                    FirstFD[1] = SecondFD[1];
                    // 关闭当前管道的写端
                    close(FirstFD[1]);
                    // 这里已经是最后一个管道符
                    // 后一个管道的读端设置为非法
                    SecondFD[0] = -1;
                    // 后一个管道的写端设置为标准输出
                    // 这样使得指令总体向标准输出中读取信息
                    SecondFD[1] = STDOUT_FILENO;
                }
                else
                {
                    // 其他的管道符，即指令之中的管道符
                    // 关闭前一个管道的读端
                    if (FirstFD[0] != STDIN_FILENO)
                        close(FirstFD[0]);
                    // 更新前一个管道的信息
                    FirstFD[0] = SecondFD[0];
                    FirstFD[1] = SecondFD[1];
                    // 关闭当前管道对应的写端
                    close(FirstFD[1]);
                    // 创建后一个管道
                    pipe(SecondFD);
                }

                // 为每一条子指令fork出新进程
                PipeBuffer[PipeCount++] = fork();
                if (PipeBuffer[PipeCount - 1] == 0)
                {
                    // 初始化每一个子进程
                    // 将信号处理函数恢复为系统默认处理函数
                    signal(SIGINT, SIG_IGN);
                    signal(SIGTSTP, SIG_DFL);
                    // 重定向标准输入和输出
                    dup2(FirstFD[0], STDIN_FILENO);
                    dup2(SecondFD[1], STDOUT_FILENO);
                    // 为了实现管道单向的数据传输，关闭对应端口
                    close(FirstFD[1]);
                    close(SecondFD[0]);
                    // 执行指令
                    ExecCommand(DividedCommand + LastPipe + 1, i - LastPipe - 1, false);
                    // 执行结束后退出
                    exit(0);
                }
                // 更新上一个管道符的位置
                LastPipe = i;
            }
        }
        close(FirstFD[0]);
        // 等待所有子进程完成
        for (int i = 0; i < PipeCount; ++i)
            waitpid(PipeBuffer[i], NULL, 0);
        exit(0);
    }
    return;
}

// ExecCommand函数负责处理输入输出和具体执行指令
void ExecCommand(string DividedCommand[], int ParaCount, bool WithFork)
{
    // 备份标准输入，标准输出和错误
    int StdInput = dup(STDIN_FILENO);
    int StdOutput = dup(STDOUT_FILENO);
    int StdError = dup(STDERR_FILENO);

    // 记录可能的重定向后三者的去向
    int Input = -1;
    int Output = -1;
    int Error = -1;

    // 更新指令执行状态
    State = 0;

    // 可能的重定向文件名
    string InputFile = "";
    string OutputFile = "";
    string ErrorFile = "";

    // 倒序扫描，使得我们可以在读取到重定向符前已经定位到文件名
    for (int i = ParaCount - 2; i >= 0; --i)
    {
        if (DividedCommand[i] == "<" || DividedCommand[i] == "0<")
        {
            // 输入重定向
            if (InputFile != "")
            {
                // 过多的输入重定向，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many input redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }

            // 载入文件名
            InputFile = DividedCommand[i + 1];
            // 打开文件
            Input = open(InputFile.c_str(), O_RDONLY);

            if (Input < 0)
            {
                // 无法打开文件，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open input redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 重定向
            dup2(Input, STDIN_FILENO);
            close(Input);
            // 将重定向部分从参数个数中删除
            ParaCount = i;
        }

        // 覆盖式输出重定向
        else if (DividedCommand[i] == ">" || DividedCommand[i] == "1>")
        {

            if (OutputFile != "")
            {
                // 过多的输出重定向，输出错误信息
                InfoToStdOutput = "";
                // 更新指令执行状态
                InfoToStdError = "ERROR: Too many output redirection.\n";
                State = 1;
                break;
            }
            // 载入文件名
            OutputFile = DividedCommand[i + 1];
            // 打开文件
            Output = open(OutputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Output < 0)
            {
                // 无法打开文件，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or creat output redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 重定向
            dup2(Output, STDOUT_FILENO);
            close(Output);
            // 将重定向部分从参数个数中删除
            ParaCount = i;
        }

        // 追加式输出重定向
        else if (DividedCommand[i] == ">>" || DividedCommand[i] == "1>>")
        {
            if (OutputFile != "")
            {
                // 过多的输出重定向，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many output redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 载入文件名
            OutputFile = DividedCommand[i + 1];
            // 打开文件
            Output = open(OutputFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);

            if (Output < 0)
            {
                // 无法打开文件，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create output redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 重定向
            dup2(Output, STDOUT_FILENO);
            close(Output);
            // 将重定向部分从参数个数中删除
            ParaCount = i;
        }
        // 覆盖式错误重定向
        else if (DividedCommand[i] == "2>")
        {
            if (ErrorFile != "")
            {
                // 过多的错误重定向，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 载入文件名
            ErrorFile = DividedCommand[i + 1];
            // 打开文件
            Error = open(ErrorFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Error < 0)
            {
                // 无法打开文件，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create error redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 重定向
            dup2(Error, STDERR_FILENO);
            close(Error);
            // 将重定向部分从参数个数中删除
            ParaCount = i;
        }
        // 追加式错误重定向
        else if (DividedCommand[i] == "2>>")
        {
            if (ErrorFile != "")
            {
                // 过多的错误重定向，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 载入文件名
            ErrorFile = DividedCommand[i + 1];
            // 打开文件
            Error = open(ErrorFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Error < 0)
            {
                // 无法打开文件，输出错误信息
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create error redirection.\n";
                // 更新指令执行状态
                State = 1;
                break;
            }
            // 重定向
            dup2(Error, STDERR_FILENO);
            close(Error);
            // 将重定向部分从参数个数中删除
            ParaCount = i;
        }
    }

    if (State == 0)
    {
        // 无重定向符或者重定向处理正常结束
        if (ParaCount == 0 || DividedCommand[0][0] == '#')
        {
            InfoToStdOutput = "";
            InfoToStdError = "";
        }
        // 调用不同的处理函数处理对应的指令
        else if (DividedCommand[0] == "cd")
            ExecCommandCd(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "exit")
            ExecCommandExit(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "clr")
            ExecCommandClr(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "time")
            ExecCommandTime(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "pwd")
            ExecCommandPwd(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "echo")
            ExecCommandEcho(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "dir")
            ExecCommandDir(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "test")
            ExecCommandTest(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "jobs")
            ExecCommandJobs(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "bg")
            ExecCommandBg(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "fg")
            ExecCommandFg(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "set")
            ExecCommandSet(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "umask")
            ExecCommandUmask(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "help")
            ExecCommandHelp(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "exec")
            // 由于exec函数的复用，所以其传参格式略有区别
            ExecCommandExec(DividedCommand + 1, ParaCount - 1);
        else
        {
            // 其他命令，视为程序调用
            InfoToStdOutput = "";
            InfoToStdError = "";
            if (WithFork)
            {
                // fork出子进程
                SonPID = fork();
                if (SonPID == 0)
                {
                    // 子进程
                    // 设置环境变量
                    setenv("PARENT", ShellDir.c_str(), 1);
                    // 调用Exec函数，此即上文中提到的复用
                    ExecCommandExec(DividedCommand, ParaCount);
                    // 执行到此处即表明程序调用出错
                    // 设置错误信息并输出
                    InfoToStdError = "ERROR: Can't execute.\n";
                    write(STDERR_FILENO, InfoToStdError.c_str(), InfoToStdError.length());
                    // 子进程退出
                    exit(0);
                }
                else
                {
                    // 父进程
                    // 等待子进程完成
                    while (SonPID != -1 && !waitpid(SonPID, NULL, WNOHANG))
                        ;
                    SonPID = -1;
                }
            }
            else
            {
                // 设置环境变量
                setenv("PARENT", ShellDir.c_str(), 1);
                // 调用Exec函数，此即上文中提到的复用
                ExecCommandExec(DividedCommand, ParaCount);
                // 执行到此处即表明程序调用出错
                InfoToStdError = "ERROR: Can't execute.\n";
            }
        }
    }

    // 向(重定向)后的标准输入输出中写入提示信息
    write(STDOUT_FILENO, InfoToStdOutput.c_str(), InfoToStdOutput.length());
    write(STDERR_FILENO, InfoToStdError.c_str(), InfoToStdError.length());
    // 恢复标准输入输出以及错误
    dup2(StdInput, STDIN_FILENO);
    dup2(StdOutput, STDOUT_FILENO);
    dup2(StdError, STDERR_FILENO);
    // 关闭文件
    close(StdInput);
    close(StdOutput);
    close(StdError);
    return;
}

// 执行cd命令
void ExecCommandCd(string DividedCommand[], int ParaCount)
{
    // 本函数主要通过chdir函数实现对目录的切换
    if (ParaCount == 1 || (ParaCount == 2 && DividedCommand[1] == "~"))
    {
        // 仅有一个参数(即cd)或第二个参数为"~"(即cd ~)
        if (chdir(HomeDir.c_str()))
        {
            // 未能成功进入目录
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't change to the target directory.\n";
            State = 1;
        }
        else
        {
            char Temp[MAX_STRING_SIZE];
            getcwd(Temp, MAX_STRING_SIZE);
            // 更新当前路径
            CurrentPath = Temp;
            // 更新环境变量中的当前路径
            setenv("PWD", Temp, 1);
            InfoToStdOutput = "";
            InfoToStdError = "";
            State = 0;
        }
    }
    else if (ParaCount == 2)
    {
        // 有两个参数(即形如cd /的指令)
        if (chdir(DividedCommand[1].c_str()))
        {
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't change to the target directory.\n";
            State = 1;
        }
        else
        {
            char Temp[MAX_STRING_SIZE];
            getcwd(Temp, MAX_STRING_SIZE);
            // 更新当前路径
            CurrentPath = Temp;
            // 更新环境变量中的当前路径
            setenv("PWD", Temp, 1);
            InfoToStdOutput = "";
            InfoToStdError = "";
            State = 0;
        }
    }
    else
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

// 执行exit命令
void ExecCommandExit(string DividedCommand[], int ParaCount)
{
    // 本函数主要通过kill函数实现退出shell
    // 获取当前进程进程号
    pid_t pid = getpid();
    // 通过kill函数终止shell
    if (kill(pid, SIGTERM) == -1)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Can't exit shell.\n";
        State = 1;
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "";
        State = 0;
    }
    return;
}

// 执行clr命令
void ExecCommandClr(string DividedCommand[], int ParaCount)
{
    // 本函数主要通过system调用实现清屏
    if (ParaCount == 1)
    {
        system("clear");
        InfoToStdOutput = "";
        InfoToStdError = "";
        State = 0;
    }
    else
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

// 执行time命令
void ExecCommandTime(string DividedCommand[], int ParaCount)
{
    // 本函数主要通过time函数获取系统时间
    // 静态常量数组，用于将数字转换为对应的字符串
    static const string Week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (ParaCount == 1)
    {
        // 通过time函数获取系统时间
        time_t NowTime = time(NULL);
        struct tm *NowTimeStruct = localtime(&NowTime);
        // 拼接结果
        InfoToStdOutput = to_string(NowTimeStruct->tm_year + 1900) + "." + to_string(NowTimeStruct->tm_mon + 1) + "." + to_string(NowTimeStruct->tm_mday);
        InfoToStdOutput += " ";
        InfoToStdOutput += Week[NowTimeStruct->tm_wday];
        InfoToStdOutput += " ";
        InfoToStdOutput += to_string(NowTimeStruct->tm_hour) + ":" + to_string(NowTimeStruct->tm_min) + ":" + to_string(NowTimeStruct->tm_sec) + "\n";
        InfoToStdError = "";
        State = 0;
    }
    else
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

// 执行pwd命令
void ExecCommandPwd(string DividedCommand[], int ParaCount)
{
    // 本函数只需要输出当前路径即可
    if (ParaCount > 1)
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else
    {
        InfoToStdOutput = CurrentPath + "\n";
        InfoToStdError = "";
        State = 0;
    }
    return;
}

// 执行echo命令
void ExecCommandEcho(string DividedCommand[], int ParaCount)
{
    // 本函数只需要将后续参数打印出来即可
    State = 0;
    InfoToStdOutput = "";
    InfoToStdError = "";

    for (int i = 1; i < ParaCount; ++i)
        if (DividedCommand[i] != "")
            InfoToStdOutput += DividedCommand[i] + " ";

    InfoToStdOutput += "\n";
    return;
}

// 执行dir命令
void ExecCommandDir(string DividedCommand[], int ParaCount)
{
    // 本函数主要通过opendir和readdir函数获取目标路径下数据并输出
    if (ParaCount > 2)
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else
    {
        string Temp;
        if (ParaCount == 1)
            // 无参，默认为枚举当前目录
            Temp = CurrentPath;
        else
            Temp = DividedCommand[1];

        DIR *TargetDir;
        struct dirent *PtrToTarget;

        if (!(TargetDir = opendir(Temp.c_str())))
        {
            // 无法打开目标目录，无法执行
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't open target.\n";
            State = 1;
        }

        // 成功打开目标目录，通过循环读取文件信息
        while ((PtrToTarget = readdir(TargetDir)) != NULL)
        {
            string Temp = PtrToTarget->d_name;
            if (Temp != "." && Temp != "..")
                // 跳过.和..
                InfoToStdOutput += Temp + "\n";
        }
        // 关闭目录
        closedir(TargetDir);
    }
    return;
}

// 执行test命令
void ExecCommandTest(string DividedCommand[], int ParaCount)
{
    int Result = 0;

    if (ParaCount < 3)
    {
        // 参数过少， 无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Miss parameter(s).\n";
        State = 1;
        return;
    }
    else if (ParaCount > 4)
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameter(s).\n";
        State = 1;
        return;
    }
    else if (ParaCount == 3)
    {
        // 单操作数校验
        struct stat Src;
        // 单操作数文件测试
        // 判定文件是否存在
        if (DividedCommand[1] == "-e")
        {
            if (lstat(DividedCommand[2].c_str(), &Src) == 0)
                Result = 1;
        }
        // 判定文件是否可读
        else if (DividedCommand[1] == "-r")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), R_OK))
                Result = 1;
        }
        // 判定文件是否可写
        else if (DividedCommand[1] == "-w")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), W_OK))
                Result = 1;
        }
        // 判定文件是否可执行
        else if (DividedCommand[1] == "-x")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), X_OK))
                Result = 1;
        }
        // 判定文件是否为空
        else if (DividedCommand[1] == "-s")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_size)
                Result = 1;
        }
        // 判定文件是否是目录文件
        else if (DividedCommand[1] == "-d")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISDIR(Src.st_mode))
                Result = 1;
        }
        // 判定文件是否是普通文件
        else if (DividedCommand[1] == "-f")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISREG(Src.st_mode))
                Result = 1;
        }
        // 判定文件是否是字符型特殊文件
        else if (DividedCommand[1] == "-c")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISCHR(Src.st_mode))
                Result = 1;
        }
        // 判定文件是否是块特殊文件
        else if (DividedCommand[1] == "-b")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISBLK(Src.st_mode))
                Result = 1;
        }

        // 单操作数字符串测试
        // 判定字符串长度是否为0
        else if (DividedCommand[1] == "-n")
        {
            if (DividedCommand[2].length())
                Result = 1;
        }
        else if (DividedCommand[1] == "-z")
        {
            if (DividedCommand[2].length() == 0)
                Result = 1;
        }
        else
            Result = 2;
    }
    else if (ParaCount == 4)
    {
        // 双操作数校验
        struct stat Src1, Src2;
        // 双操作数文件校验
        // 判定两个文件inode号是否相同
        if (DividedCommand[1] == "-ef")
        {
            int Ret1 = lstat(DividedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DividedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && Src1.st_dev == Src2.st_dev && Src1.st_ino == Src2.st_ino)
                Result = 1;
        }
        // 比较两个文件的最后修改时间
        else if (DividedCommand[1] == "-nt")
        {
            int Ret1 = lstat(DividedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DividedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && TimeCommpare(Src1.st_mtim, Src2.st_mtim) == 1)
                Result = 1;
        }
        else if (DividedCommand[1] == "-ot")
        {
            int Ret1 = lstat(DividedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DividedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && TimeCommpare(Src1.st_mtim, Src2.st_mtim) == -1)
                Result = 1;
        }

        // 双操作数字符串测试
        // 比较两个字符串是否相同
        else if (DividedCommand[1] == "=")
        {
            if (DividedCommand[2] == DividedCommand[3])
                Result = 1;
        }
        else if (DividedCommand[1] == "!=")
        {
            if (DividedCommand[2] != DividedCommand[3])
                Result = 1;
        }

        // 双操作数数值测试
        // 比较两数大小关系
        else if (DividedCommand[1] == "-eq")
        {
            if (atol(DividedCommand[2].c_str()) == atol(DividedCommand[3].c_str()))
                Result = 1;
        }
        else if (DividedCommand[1] == "-ge")
        {
            if (atol(DividedCommand[2].c_str()) >= atol(DividedCommand[3].c_str()))
                Result = 1;
        }
        else if (DividedCommand[1] == "-gt")
        {
            if (atol(DividedCommand[2].c_str()) > atol(DividedCommand[3].c_str()))
                Result = 1;
        }
        else if (DividedCommand[1] == "-le")
        {
            if (atol(DividedCommand[2].c_str()) <= atol(DividedCommand[3].c_str()))
                Result = 1;
        }
        else if (DividedCommand[1] == "-lt")
        {
            if (atol(DividedCommand[2].c_str()) < atol(DividedCommand[3].c_str()))
                Result = 1;
        }
        else
            Result = 2;
    }

    // 根据执行结果返回执行信息
    switch (Result)
    {
    // 返回false
    case 0:
        InfoToStdOutput = "False.\n";
        InfoToStdError = "";
        State = 0;
        break;
    // 返回true
    case 1:
        InfoToStdOutput = "True.\n";
        InfoToStdError = "";
        State = 0;
        break;
    // 无法处理的指令
    case 2:
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Unknown opinion.\n";
        State = 1;
        break;
    }
    return;
}

// 执行jobs命令
void ExecCommandJobs(string DividedCommand[], int ParaCount)
{
    // 本指令只需要将所有子进程相关信息输出即可
    if (ParaCount > 1)
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameter(s).\n";
        State = 1;
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "";
        // 循环遍历，提取所有子进程相关信息
        for (int i = Head; i < Tail; ++i)
            InfoToStdOutput += ReturnJobsInfo(i);
        State = 0;
    }
    return;
}

// 执行bg命令
void ExecCommandBg(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount == 1)
    {
        // 无参运行，输出所有正在执行或被挂起的任务信息
        for (int i = Head; i < Tail; ++i)
            if (States[i] == 1)
                InfoToStdOutput += ReturnJobsInfo(i);

        if (InfoToStdOutput == "")
            InfoToStdOutput = "No mission is running at the background.\n";
        State = 0;
        return;
    }
    // 根据传入的任务号，将对应的任务移至后台运行
    State = 0;
    for (int i = 1; i < ParaCount; ++i)
    {
        int WorkID;
        WorkID = atol(DividedCommand[i].c_str());

        if (WorkID == 0)
        {
            // 非法的任务号，处理下一个参数
            InfoToStdOutput = "Invalid ID.\n";
            continue;
        }
        else if (WorkID > Tail || WorkID <= Head || States[WorkID - 1] == 0)
        {
            // 找不到对应任务，处理下一个参数
            InfoToStdOutput = "Can't find the target mission.\n";
            continue;
        }
        else if (States[WorkID - 1] == 1)
        {
            // 任务已在后台运行，处理下一个参数
            InfoToStdOutput = "Target is already running at the background.\n";
            continue;
        }
        else
        {
            // 更新状态表
            States[WorkID - 1] = 1;
            // 向对应子进程发出信号
            kill(Jobs[WorkID - 1], SIGCONT);
        }
    }
    return;
}

// 执行fg命令
void ExecCommandFg(string DividedCommand[], int ParaCount)
{
    if (ParaCount == 1)
    {
        // 缺失参数，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Miss parameters.\n";
        State = 1;
    }
    else if (ParaCount > 2)
    {
        // 参数过多，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else
    {
        // 寻找对应的任务
        int WorkID;
        WorkID = atol(DividedCommand[1].c_str());
        if (WorkID == 0)
        {
            // 非法的任务号，无法执行
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Invalid ID.\n";
            State = 1;
        }
        else if (WorkID > Tail || WorkID <= Head || States[WorkID - 1] == 0)
        {
            // 找不到对应任务，无法执行
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't find target mission.\n";
            State = 1;
        }
        else
        {
            InfoToStdOutput = "";
            InfoToStdError = "";
            State = 0;
            NowCommand = CMDofSons[WorkID - 1];
            if (IsInTerminal)
            {
                // 将指令内容输出到终端中
                write(TerminalOut, CMDofSons[WorkID - 1].c_str(), CMDofSons[WorkID - 1].length());
                write(TerminalOut, "\n", 1);
            }
            // 更新状态表和子进程号
            States[WorkID - 1] = 0;
            SonPID = Jobs[WorkID - 1];
            // 更新任务表
            if (Tail == WorkID && Head == WorkID - 1)
                Tail = Head = 0;
            else if (Head == WorkID - 1)
                ++Head;
            else if (Tail == WorkID)
                --Tail;

            // 将任务转入前台
            setpgid(SonPID, getgid());
            // 向对应子进程发送信号
            kill(SonPID, SIGCONT);
            // 等待子进程完成
            while (SonPID != -1 && !waitpid(SonPID, NULL, WNOHANG))
                ;
            // 更新子进程进程号
            SonPID = -1;
        }
    }
}

// 执行set命令
void ExecCommandSet(string DividedCommand[], int ParaCount)
{
    // 获取环境变量表
    extern char **environ;
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount == 1)
    {
        // 循环变量，输出所有环境变量
        for (int i = 0; environ[i] != NULL; ++i)
            InfoToStdOutput = InfoToStdOutput + environ[i] + "\n";
    }
    else
    {
        // 参数个数不正确，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

// 执行umask命令
void ExecCommandUmask(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount > 2)
    {
        // 参数过多，无法执行
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else if (ParaCount == 1)
    {
        // 仅一个参数，输出当前的umask值
        // 获取umask值
        mode_t CurrentMode = umask(0);
        umask(CurrentMode);
        // 转换为字符串并输出
        InfoToStdOutput = ReturnUmaskValue(CurrentMode);
        State = 0;
    }
    else
    {
        // 两个参数，设置umask值
        if (DividedCommand[1].length() > 4)
        {
            // 用户想要设置的新值过长，无法执行
            InfoToStdError = "ERROR: At most 4 octonary.\n";
            State = 1;
        }
        else
        {
            // 获取新的umask值
            int NewMode = GetOctValue(DividedCommand[1]);
            if (NewMode < 0)
            {
                // 传入的新umask值是非法数值，无法执行
                InfoToStdError = "ERROR: Invalid umask value.\n";
                State = 1;
            }
            else
            {
                // 设置新umask值
                umask((mode_t)NewMode);
                State = 0;
            }
        }
    }
    return;
}

// 执行exec命令或者程序调用
void ExecCommandExec(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount == 0)
    {
        // 缺失参数，无法执行
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Miss parameters.\n";
        State = 1;
    }
    else
    {
        // 设置调用的参数
        char *TempArgv[ParaCount + 1] = {NULL};
        for (int i = 0; i < ParaCount; ++i)
            TempArgv[i] = const_cast<char *>(DividedCommand[i].c_str());
        TempArgv[ParaCount] = NULL;
        State = 0;
        // 通过execvp执行
        execvp(DividedCommand[0].c_str(), TempArgv);
        // 执行到此处即表明程序调用出错
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Can't execute.\n";
        State = 1;
    }
    return;
}

void ExecCommandHelp(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount <= 2)
    {
        int Begin, End;
        if (ParaCount == 1)
        {
            Begin = 1;
            End = 17;
        }
        else if (DividedCommand[1] == "bg")
        {
            Begin = 19;
            End = 23;
        }
        else if (DividedCommand[1] == "cd")
        {
            Begin = 25;
            End = 29;
        }
        else if (DividedCommand[1] == "clr")
        {
            Begin = 31;
            End = 34;
        }
        else if (DividedCommand[1] == "dir")
        {
            Begin = 36;
            End = 40;
        }
        else if (DividedCommand[1] == "echo")
        {
            Begin = 42;
            End = 45;
        }
        else if (DividedCommand[1] == "exec")
        {
            Begin = 47;
            End = 50;
        }
        else if (DividedCommand[1] == "exit")
        {
            Begin = 52;
            End = 55;
        }
        else if (DividedCommand[1] == "fg")
        {
            Begin = 57;
            End = 60;
        }
        else if (DividedCommand[1] == "set")
        {
            Begin = 62;
            End = 65;
        }
        else if (DividedCommand[1] == "help")
        {
            Begin = 67;
            End = 71;
        }
        else if (DividedCommand[1] == "jobs")
        {
            Begin = 73;
            End = 76;
        }
        else if (DividedCommand[1] == "pwd")
        {
            Begin = 78;
            End = 81;
        }
        else if (DividedCommand[1] == "set")
        {
            Begin = 83;
            End = 86;
        }
        else if (DividedCommand[1] == "test")
        {
            Begin = 88;
            End = 116;
        }
        else if (DividedCommand[1] == "time")
        {
            Begin = 118;
            End = 121;
        }
        else if (DividedCommand[1] == "umask")
        {
            Begin = 123;
            End = 127;
        }
        else
        {
            InfoToStdError = "ERROR: Unknown command.\n";
            State = 1;
            return;
        }

        int i = 0;
        ifstream File;
        string temp;
        File.open(HelpPath);
        if (!File.is_open())
        {
            InfoToStdError = "ERROR: Can't open file.\n";
            State = 1;
            return;
        }
        else
        {
            while(getline(File, temp))
            {
                ++i;
                if (i >= Begin && i <= End)
                    InfoToStdOutput += temp + "\n";
            }
        }
    }
    else
    {
        // 参数过多，无法执行
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}