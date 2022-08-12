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

using namespace std;

#define MAX_WORK_COUNT 1024
#define MAX_STRING_SIZE 1024
#define MAX_ARGV_COUNT 1024

char Buffer[MAX_STRING_SIZE];
string NowCommand;
bool IsInTerminal;
bool IsOutTerminal;
int TerminalIn;
int TerminalOut;

int State;
string ToOut;
string ToError;

string HostName;
string UserName;
string HomeDir;
string ShellDir;
string HelpPath;
string CurrentPath;

string InfoToStdOutput;
string InfoToStdError;

int PID;
int SonPID;
int _Argc;
string _Argv[MAX_ARGV_COUNT];
int Jobs[MAX_WORK_COUNT];
int States[MAX_WORK_COUNT];
string CMDofSons[MAX_WORK_COUNT];

int Head;
int Tail;

void SignalDeal(int Signal);
void InitShell(int __Argc, char **__Argv);
string ReturnJobsInfo(int JobIndex, int Finished);
int TimeCommpare(const timespec& Time1, const timespec& Time2);
void UpdateJobs();
void AnalyseCommand(string Command);
void DealIOCommand(string DevidedCommand[], int ParaCount);
void ExecCommand(string DevidedCommand[], int ParaCount);
void ExecCommandCd(string DevidedCommand[], int ParaCount);
void ExecCommandExit(string DevidedCommand[], int ParaCount);
void ExecCommandClr(string DevidedCommand[], int ParaCount);
void ExecCommandTime(string DevidedCommand[], int ParaCount);
void ExecCommandPwd(string DevidedCommand[], int ParaCount);
void ExecCommandEcho(string DevidedCommand[], int ParaCount);
void ExecCommandDir(string DevidedCommand[], int ParaCount);
void ExecCommandTest(string DevidedCommand[], int ParaCount);

int main(int Argc, char *Argv[])
{
    
    InitShell(Argc, Argv);
    int Flag = 1;
    while (Flag)
    {
        if (IsInTerminal)
        {
            sprintf(Buffer, "\e[1;32m%s@%s\e[0m:\e[1;34m%s\e[0m$ ",
                    UserName.c_str(),
                    HostName.c_str(),
                    CurrentPath.c_str());
            string Temp = Buffer;
            write(TerminalOut, Temp.c_str(), Temp.length());
        }

        int i;
        for (i = 0; ; ++i)
        {
            if(read(STDIN_FILENO, Buffer + i, 1) <= 0)
            {
                Flag = 0;
                break;
            }
            if (Buffer[i] == '\n')
                break;
        }
        Buffer[i] = '\0';
        AnalyseCommand(Buffer);
    }
    return 0;
}

string ReturnJobsInfo(int JobIndex, int Finished = 0)
{
    string Result = "";
    if (!(JobIndex < Head || JobIndex >= Tail))
    {
        Result = "[" + to_string(JobIndex + 1) + "]\t" + to_string(Jobs[JobIndex]) + "\t";
        if (Finished)
            Result += "Finished";
        else if (States[JobIndex] == 1)
            Result += "Running";
        else
            Result += "Hanging";

        Result += "\t";
        Result += CMDofSons[JobIndex];
        Result += "\n";
    }
    return Result;
}

void SignalDeal(int Signal)
{
    switch (Signal)
    {
    case SIGINT:
        write(TerminalOut, "\n", 1);
        break;
    case SIGTSTP:
        write(TerminalOut, "\n", 1);
        if (SonPID != -1)
        {
            setpgid(SonPID, 0);
            kill(SonPID, SIGTSTP);
            Jobs[Tail] = SonPID;
            States[Tail] = 2;
            CMDofSons[Tail] = NowCommand;

            if (IsInTerminal)
            {
                string Resuult = ReturnJobsInfo(Tail);
                write(TerminalOut, Resuult.c_str(), Resuult.length());
            }
            Tail++;
            SonPID = -1;
        }
        break;
    case SIGCONT:
        break;
    default:
        break;
    }
}

int TimeCommpare(const timespec& Time1, const timespec& Time2)
{
    if (Time1.tv_sec > Time2.tv_sec)
        return 1;
    else if (Time1.tv_sec < Time2.tv_sec)
        return -1;
    else if(Time1.tv_nsec > Time2.tv_nsec)
        return 1;
    else if (Time1.tv_nsec < Time2.tv_nsec)
        return -1;
    else
        return 0;
}

void InitShell(int __Argc, char **__Argv)
{

    char Buffer[MAX_STRING_SIZE] = {0};
    _Argc = __Argc;
    for (int i = 0; i < __Argc; ++i)
        _Argv[i] = __Argv[i];

    int InputSrc = -1;
    if (__Argc > 1)
    {
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
    PID = getpid();
    SonPID = -1;

    gethostname(Buffer, MAX_STRING_SIZE);
    HostName = Buffer;
    UserName = getenv("USERNAME");
    HomeDir = getenv("HOME");
    CurrentPath = getenv("PWD");

    HelpPath = CurrentPath + "/help";

    int Length = readlink("/proc/self/exe", Buffer, MAX_STRING_SIZE);
    Buffer[Length] = '\0';
    ShellDir = Buffer;
    setenv("SHELL", Buffer, 1);
    setenv("PARENT", "\\bin\\bash", 1);

    Head = Tail = 0;
    TerminalIn = open("/dev/tty", O_RDONLY);
    TerminalOut = open("/dev/tty", O_WRONLY);

    struct stat FileInfo;
    fstat(STDIN_FILENO, &FileInfo);
    IsInTerminal = S_ISCHR(FileInfo.st_mode);
    fstat(STDOUT_FILENO, &FileInfo);
    IsOutTerminal = S_ISCHR(FileInfo.st_mode);

    if (IsInTerminal)
    {
        signal(SIGINT, SignalDeal);
        signal(SIGTSTP, SignalDeal);
    }
    return;
}

void UpdateJobs()
{
    for (int i = Head; i < Tail; ++i)
    {
        if (States[i] && Jobs[i] == waitpid(Jobs[i], NULL, WNOHANG))
        {
            if (IsInTerminal)
            {
                string Temp = ReturnJobsInfo(i, 1);
                write(TerminalOut, Temp.c_str(), Temp.length());
            }
            States[i] = 0;
            if (Head == i)
                ++Head;
        }      
    }
    if (Head == Tail)
        Head = Tail = 0;
    return;
}

void AnalyseCommand(string Command)
{
    UpdateJobs();

    stringstream TempStream;
    int ParaCount = 0;
    string DividedCommand[MAX_ARGV_COUNT];
    TempStream << Command;

    while(true)
    {
        DividedCommand[ParaCount] = "";
        TempStream >> DividedCommand[ParaCount];
        if (DividedCommand[ParaCount] == "")
            break;
        ++ParaCount;
    }
    if (ParaCount > 0 && DividedCommand[ParaCount - 1] == "&")
    {
        return;
    }
    else
    {
        NowCommand = Command;
        DealIOCommand(DividedCommand, ParaCount);
        return;
    }
}

void DealIOCommand(string DevidedCommand[], int ParaCount)
{
    bool PipeExistsFlag = false;

    if(PipeExistsFlag == false)
    {
        ExecCommand(DevidedCommand, ParaCount);
        return;
    }
    else
        return;
}

void ExecCommand(string DevidedCommand[], int ParaCount)
{
    int StdInput = dup(STDIN_FILENO);
    int StdOutput = dup(STDOUT_FILENO);
    int StdError = dup(STDERR_FILENO);

    int Input = -1;
    int Output = -1;
    int Error = -1;

    State = 0;

    string InputFile = "";
    string OutputFile = "";
    string ErrorFile = "";

    for (int i = ParaCount - 2; i >= 0; --i)
    {
        if (DevidedCommand[i] == "<" || DevidedCommand[i] == "0<")
        {
            if (InputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many input redirection.\n";
                State = 1;
                break;
            }

            InputFile = DevidedCommand[i + 1];
            Input = open(InputFile.c_str(), O_RDONLY);

            if (Input < 0)
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open input redirection.\n";
                State = 1;
                break;
            }
            
            dup2(Input, STDIN_FILENO);
            close(Input);
            ParaCount = i;
        }
        
        else if (DevidedCommand[i] == ">" || DevidedCommand[i] == "1>")
        {
            if (OutputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many output redirection.\n";
                State = 1;
                break;
            }
            
            OutputFile = DevidedCommand[i + 1];
            Output = open(OutputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Output < 0)
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or creat output redirection.\n";
                State = 1;
                break;
            }

            dup2(Output, STDOUT_FILENO);
            close(Output);
            ParaCount = i;
        }

        else if (DevidedCommand[i] == ">>" || DevidedCommand[i] == "1>>")
        {
            if (OutputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many output redirection.\n";
                State = 1;
                break;
            }
            
            OutputFile = DevidedCommand[i + 1];
            Output = open(OutputFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);

            if (Output < 0)
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create output redirection.\n";
                State = 1;
                break;
            }

            dup2(Output, STDOUT_FILENO);
            close(Output);
            ParaCount = i;
        }
        
        else if (DevidedCommand[i] == "2>")
        {
            if (ErrorFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                State = 1;
                break;
            }
            
            ErrorFile = DevidedCommand[i + 1];
            Error = open(ErrorFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Error < 0)
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create error redirection.\n";
                State = 1;
                break;
            }

            dup2(Error, STDERR_FILENO);
            close(Error);
            ParaCount = i;
        }
        
        else if (DevidedCommand[i] == "2>>")
        {
            if (ErrorFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                State = 1;
                break;
            }
            
            ErrorFile = DevidedCommand[i + 1];
            Error = open(ErrorFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            if (Error < 0)
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Can't open or create error redirection.\n";
                State = 1;
                break;
            }

            dup2(Error, STDERR_FILENO);
            close(Error);
            ParaCount = i;
        }
    }

    if (State == 0)
    {
        if (ParaCount == 0 || DevidedCommand[0][0] == '#')
        {
            InfoToStdOutput = "";
            InfoToStdError = "";
        }
        else if (DevidedCommand[0] == "cd")
            ExecCommandCd(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "exit")
            ExecCommandExit(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "clear")
            ExecCommandClr(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "time")
            ExecCommandTime(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "pwd")
            ExecCommandPwd(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "echo")
            ExecCommandEcho(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "dir")
            ExecCommandDir(DevidedCommand, ParaCount);
        else if (DevidedCommand[0] == "test")
            ExecCommandTest(DevidedCommand, ParaCount);
        
    }

    write(STDOUT_FILENO, InfoToStdOutput.c_str(), InfoToStdOutput.length());
    write(STDERR_FILENO, InfoToStdError.c_str(), InfoToStdError.length());
    dup2(StdInput, STDIN_FILENO);
    dup2(StdOutput, STDOUT_FILENO);
    dup2(StdError, STDERR_FILENO);

    close(StdInput);
    close(StdOutput);
    close(StdError);
    return;
}

void ExecCommandCd(string DevidedCommand[], int ParaCount)
{
    if (ParaCount == 1 || (ParaCount == 2 && DevidedCommand[1] == "~"))
    {
        if (chdir(HomeDir.c_str()))
        {
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't change to the target directory.\n";
            State = 1;
        }
        else
        {
            char Temp[MAX_STRING_SIZE];
            getcwd(Temp, MAX_STRING_SIZE);
            CurrentPath = Temp;
            setenv("PWD", Temp, 1);
            InfoToStdOutput = "";
            InfoToStdError = "";
            State = 0;
        }
    }
    else if (ParaCount == 2)
    {
        if (chdir(DevidedCommand[1].c_str()))
        {
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't change to the target directory.\n";
            State = 1;
        }
        else
        {
            char Temp[MAX_STRING_SIZE];
            getcwd(Temp, MAX_STRING_SIZE);
            CurrentPath = Temp;
            setenv("PWD", Temp, 1);
            InfoToStdOutput = "";
            InfoToStdError = "";
            State = 0;
        }
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

void ExecCommandExit(string DevidedCommand[], int ParaCount)
{
    pid_t pid = getpid();
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

void ExecCommandClr(string DevidedCommand[], int ParaCount)
{
    if (ParaCount == 1)
    {
        system("clear");
        InfoToStdOutput = "";
        InfoToStdError = "";
        State = 0;
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

void ExecCommandTime(string DevidedCommand[], int ParaCount)
{
    static const string Week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (ParaCount == 1)
    {
        time_t NowTime = time(NULL);
        struct tm *NowTimeStruct = localtime(&NowTime);
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
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

void ExecCommandPwd(string DevidedCommand[], int ParaCount)
{
    if (ParaCount > 1)
    {
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

void ExecCommandEcho(string DevidedCommand[], int ParaCount)
{
    State = 0;
    InfoToStdOutput = "";
    InfoToStdError = "";

    for (int i = 1; i < ParaCount; ++i)
        if (DevidedCommand[i] != "")
            InfoToStdOutput += DevidedCommand[i] + " ";
    
    InfoToStdOutput += "\n";
    return;
}

void ExecCommandDir(string DevidedCommand[], int ParaCount)
{
    if (ParaCount > 2)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else
    {
        string Temp;
        if (ParaCount == 1)
            Temp = CurrentPath;
        else
            Temp = DevidedCommand[1];
        
        DIR *TargetDir;
        struct dirent *PtrToTarget;

        if (!(TargetDir = opendir(Temp.c_str())))
        {
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Can't open target.\n";
            State = 1;
        }
        
        while((PtrToTarget = readdir(TargetDir)) != NULL)
        {
            string Temp = PtrToTarget->d_name;
            if (Temp != "." && Temp != "..")
                InfoToStdOutput += Temp + "\n";
        }
        closedir(TargetDir);
    }
    return;
}

void ExecCommandTest(string DevidedCommand[], int ParaCount)
{
    int Result = 0;
    
    if (ParaCount < 3)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Miss parameter(s).\n";
        State = 1;
        return;
    }
    else if (ParaCount > 4)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameter(s).\n";
        State = 1;
        return;
    }
    else if (ParaCount == 3)
    {
        struct stat Src;
        if (DevidedCommand[1] == "-e")
        {
            if (lstat(DevidedCommand[2].c_str(), &Src) == 0)
                Result = 1;
        }
        else if (DevidedCommand[1] == "-r")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DevidedCommand[1].c_str(), R_OK))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-w")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DevidedCommand[1].c_str(), W_OK))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-x")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DevidedCommand[1].c_str(), X_OK))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-s")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_size)
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-d")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISDIR(Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-f")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISREG(Src.st_mode))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-c")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISCHR(Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-b")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISBLK(Src.st_mode))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-b")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISBLK(Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-h" || DevidedCommand[1] == "-L")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISLNK(Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-p")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISFIFO(Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-S")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISFIFO(Src.st_mode))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-G")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_gid == getgid())
                Result = 1;
        }

        else if (DevidedCommand[1] == "-O")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_uid == getuid())
                Result = 1;
        }

        else if (DevidedCommand[1] == "-g")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISGID & Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-u")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISUID & Src.st_mode))
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-k")
        {
            int flag = lstat(DevidedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISVTX & Src.st_mode))
                Result = 1;
        }

        else if (DevidedCommand[1] == "-n")
        {
            if (DevidedCommand[2].length())
                Result = 1;
        }
        
        else if (DevidedCommand[1] == "-n")
        {
            if (DevidedCommand[2].length() == 0)
            Result = 1;
        }
        else
            Result = 2;
    }
    else if (ParaCount == 4)
    {
        struct stat Src1, Src2;
        if (DevidedCommand[1] == "-ef")
        {
            int Ret1 = lstat(DevidedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DevidedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && Src1.st_dev == Src2.st_dev && Src1.st_ino == Src2.st_ino)
                Result = 1;
        }
        else if (DevidedCommand[1] == "-nt")
        {
            int Ret1 = lstat(DevidedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DevidedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && TimeCommpare(Src1.st_mtim, Src2.st_mtim) == 1)
                Result = 1;
        }
        else if (DevidedCommand[1] == "-ot")
        {
            int Ret1 = lstat(DevidedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DevidedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && TimeCommpare(Src1.st_mtim, Src2.st_mtim) == -1)
                Result = 1;
        }
        else
            Result = 2;
    }
    
    switch(Result)
    {
        case 0:
            InfoToStdOutput = "False.\n";
            InfoToStdError = "";
            State = 0;
            break;
        case 1:
            InfoToStdOutput = "True.\n";
            InfoToStdError = "";
            State = 0;
            break;
        
        case 2:
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Unknown opinion.\n";
            State = 1;
            break;
    }
    return;
}