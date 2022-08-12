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
#define MAX_PARA_COUNT 1024
#define IS_OCT_DIGIT(SRC) (SRC >= '0' && SRC <= '7')

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
string _Argv[MAX_PARA_COUNT];
int Jobs[MAX_WORK_COUNT];
int States[MAX_WORK_COUNT];
string CMDofSons[MAX_WORK_COUNT];

int Head;
int Tail;

void SignalDeal(int Signal);
void InitShell(int __Argc, char **__Argv);
string ReturnJobsInfo(int JobIndex, int Finished);
string ReturnUmaskValue(mode_t CurrentMode);
int GetOctValue(string Src);
int TimeCommpare(const timespec& Time1, const timespec& Time2);
void UpdateJobs();
void AnalyseCommand(string Command);
void DealIOCommand(string DividedCommand[], int ParaCount, bool WithFork = true);
void ExecCommand(string DividedCommand[], int ParaCount, bool WithFork = true);
void ExecCommandCd(string DividedCommand[], int ParaCount);
void ExecCommandExit(string DividedCommand[], int ParaCount);
void ExecCommandClr(string DividedCommand[], int ParaCount);
void ExecCommandTime(string DividedCommand[], int ParaCount);
void ExecCommandPwd(string DividedCommand[], int ParaCount);
void ExecCommandEcho(string DividedCommand[], int ParaCount);
void ExecCommandDir(string DividedCommand[], int ParaCount);
void ExecCommandTest(string DividedCommand[], int ParaCount);
void ExecCommandJobs(string DividedCommand[], int ParaCount);
void ExecCommandBg(string DividedCommand[], int ParaCount);
void ExecCommandFg(string DividedCommand[], int ParaCount);
void ExecCommandSet(string DividedCommand[], int ParaCount);
void ExecCommandUmask(string DividedCommand[], int ParaCount);

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

string ReturnUmaskValue(mode_t CurrentMode)
{
    string Result = "";
    Result += to_string((CurrentMode >> 9) & 7);
    Result += to_string((CurrentMode >> 6) & 7);
    Result += to_string((CurrentMode >> 3) & 7);
    Result += to_string(CurrentMode & 7);
    Result += "\n";
    return Result;
}

int GetOctValue(string Src)
{
    int Result = 0;
    for (int i = 0; i < Src.length(); ++i)
    {
        if (!IS_OCT_DIGIT(Src[i]))
        {
            Result = -1;
            break;
        }
        Result = Result * 8 + (Src[i] - '0');
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
                string Result = ReturnJobsInfo(Tail);
                write(TerminalOut, Result.c_str(), Result.length());
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
    string DividedCommand[MAX_PARA_COUNT];
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
        int pid = fork();
        if (pid)
        {
            Jobs[Tail] = pid;
            States[Tail] = 1;
            CMDofSons[Tail] = "";

            for (int i = 0; i < ParaCount - 1; ++i)
                CMDofSons[Tail] += DividedCommand[i] + "";
            
            NowCommand = CMDofSons[Tail];
            if (IsInTerminal)
            {
                string Temp = ReturnJobsInfo(Tail);
                write(TerminalOut, Temp.c_str(), Temp.length());
            }
            ++Tail;
        }
        else
        {
            setpgid(0, 0);
            DealIOCommand(DividedCommand, ParaCount - 1, false);
            exit(0);
        }
    }
    else
    {
        NowCommand = Command;
        DealIOCommand(DividedCommand, ParaCount);
        return;
    }
}

void DealIOCommand(string DividedCommand[], int ParaCount, bool WithFork)
{
    bool PipeExistsFlag = false;
    for (int i = 0; i < ParaCount; ++i)
    {
        if (DividedCommand[i] == "|")
        {
            PipeExistsFlag = true;
            break;
        }
    }
    if(PipeExistsFlag == false)
    {
        ExecCommand(DividedCommand, ParaCount);
        return;
    }
    
    if (WithFork)
        SonPID = fork();
    
    if (WithFork && SonPID)
    {
        while(SonPID != -1 && !waitpid(SonPID, NULL, WNOHANG));
        SonPID = -1;
    }
    else
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        int LastPipe = -1;

        int FirstFD[2], SecondFD[2];

        int PipeBuffer[MAX_PARA_COUNT];
        int PipeCNT = 0;

        DividedCommand[ParaCount++] = "|";

        for (int i = 0; i < ParaCount; ++i)
        {
            if (DividedCommand[i] == "|")
            {
                if (LastPipe == -1)
                {
                    FirstFD[0] = STDIN_FILENO;
                    FirstFD[1] = -1;
                    pipe(SecondFD);
                }
                else if (i == ParaCount - 1)
                {
                    if (FirstFD[0] != STDIN_FILENO)
                        close(FirstFD[0]);

                    FirstFD[0] = SecondFD[0];
                    FirstFD[1] = SecondFD[1];
                    close(FirstFD[1]);
                    SecondFD[0] = -1;
                    SecondFD[1] = STDOUT_FILENO;
                }
                else
                {
                    if (FirstFD[0] != STDIN_FILENO)
                        close(FirstFD[0]);
                    FirstFD[0] = SecondFD[0];
                    FirstFD[1] = SecondFD[1];
                    close(FirstFD[1]);
                    pipe(SecondFD);
                }
                
                PipeBuffer[PipeCNT++] = fork();
                if (PipeBuffer[PipeCNT - 1] == 0)
                {
                    signal(SIGINT, SIG_IGN);
                    signal(SIGTSTP, SIG_DFL);
                    dup2(FirstFD[0], STDIN_FILENO);
                    dup2(SecondFD[1], STDOUT_FILENO);
                    close(FirstFD[1]);
                    close(SecondFD[0]);
                    ExecCommand(DividedCommand + LastPipe + 1, i - LastPipe - 1, false);
                    exit(0);
                }
                LastPipe = i;
            }
        }
        
        close(FirstFD[0]);
        for (int i = 0; i < PipeCNT; ++i)
            waitpid(PipeBuffer[i], NULL, 0);
        exit(0);
    }
    return;
}

void ExecCommand(string DividedCommand[], int ParaCount, bool WithFork)
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
        if (DividedCommand[i] == "<" || DividedCommand[i] == "0<")
        {
            if (InputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many input redirection.\n";
                State = 1;
                break;
            }

            InputFile = DividedCommand[i + 1];
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
        
        else if (DividedCommand[i] == ">" || DividedCommand[i] == "1>")
        {
            if (OutputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many output redirection.\n";
                State = 1;
                break;
            }
            
            OutputFile = DividedCommand[i + 1];
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

        else if (DividedCommand[i] == ">>" || DividedCommand[i] == "1>>")
        {
            if (OutputFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many output redirection.\n";
                State = 1;
                break;
            }
            
            OutputFile = DividedCommand[i + 1];
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
        
        else if (DividedCommand[i] == "2>")
        {
            if (ErrorFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                State = 1;
                break;
            }
            
            ErrorFile = DividedCommand[i + 1];
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
        
        else if (DividedCommand[i] == "2>>")
        {
            if (ErrorFile != "")
            {
                InfoToStdOutput = "";
                InfoToStdError = "ERROR: Too many error redirection.\n";
                State = 1;
                break;
            }
            
            ErrorFile = DividedCommand[i + 1];
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
        if (ParaCount == 0 || DividedCommand[0][0] == '#')
        {
            InfoToStdOutput = "";
            InfoToStdError = "";
        }
        else if (DividedCommand[0] == "cd")
            ExecCommandCd(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "exit")
            ExecCommandExit(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "clear")
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
        else if (DividedCommand[0] == "set")
            ExecCommandSet(DividedCommand, ParaCount);
        else if (DividedCommand[0] == "umask")
            ExecCommandUmask(DividedCommand, ParaCount);
        
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

void ExecCommandCd(string DividedCommand[], int ParaCount)
{
    if (ParaCount == 1 || (ParaCount == 2 && DividedCommand[1] == "~"))
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

void ExecCommandExit(string DividedCommand[], int ParaCount)
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

void ExecCommandClr(string DividedCommand[], int ParaCount)
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

void ExecCommandTime(string DividedCommand[], int ParaCount)
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

void ExecCommandPwd(string DividedCommand[], int ParaCount)
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

void ExecCommandEcho(string DividedCommand[], int ParaCount)
{
    State = 0;
    InfoToStdOutput = "";
    InfoToStdError = "";

    for (int i = 1; i < ParaCount; ++i)
        if (DividedCommand[i] != "")
            InfoToStdOutput += DividedCommand[i] + " ";
    
    InfoToStdOutput += "\n";
    return;
}

void ExecCommandDir(string DividedCommand[], int ParaCount)
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
            Temp = DividedCommand[1];
        
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

void ExecCommandTest(string DividedCommand[], int ParaCount)
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
        if (DividedCommand[1] == "-e")
        {
            if (lstat(DividedCommand[2].c_str(), &Src) == 0)
                Result = 1;
        }
        else if (DividedCommand[1] == "-r")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), R_OK))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-w")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), W_OK))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-x")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && access(DividedCommand[1].c_str(), X_OK))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-s")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_size)
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-d")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISDIR(Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-f")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISREG(Src.st_mode))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-c")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISCHR(Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-b")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISBLK(Src.st_mode))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-b")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISBLK(Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-h" || DividedCommand[1] == "-L")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISLNK(Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-p")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISFIFO(Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-S")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && S_ISFIFO(Src.st_mode))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-G")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_gid == getgid())
                Result = 1;
        }

        else if (DividedCommand[1] == "-O")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && Src.st_uid == getuid())
                Result = 1;
        }

        else if (DividedCommand[1] == "-g")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISGID & Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-u")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISUID & Src.st_mode))
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-k")
        {
            int flag = lstat(DividedCommand[2].c_str(), &Src);
            if (flag == 0 && (S_ISVTX & Src.st_mode))
                Result = 1;
        }

        else if (DividedCommand[1] == "-n")
        {
            if (DividedCommand[2].length())
                Result = 1;
        }
        
        else if (DividedCommand[1] == "-n")
        {
            if (DividedCommand[2].length() == 0)
            Result = 1;
        }
        else
            Result = 2;
    }
    else if (ParaCount == 4)
    {
        struct stat Src1, Src2;
        if (DividedCommand[1] == "-ef")
        {
            int Ret1 = lstat(DividedCommand[2].c_str(), &Src1);
            int Ret2 = lstat(DividedCommand[3].c_str(), &Src2);
            if (Ret1 == 0 && Ret2 == 0 && Src1.st_dev == Src2.st_dev && Src1.st_ino == Src2.st_ino)
                Result = 1;
        }
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

void ExecCommandJobs(string DividedCommand[], int ParaCount)
{
    if (ParaCount > 1)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameter(s).\n";
        State = 1;
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "";
        for (int i = Head; i < Tail; ++i)
            InfoToStdOutput += ReturnJobsInfo(i);
        State = 0;
    }
    return;
}

void ExecCommandBg(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount == 1)
    {
        for (int i = Head; i < Tail; ++i)
            if (States[i] == 1)
                InfoToStdOutput += ReturnJobsInfo(i);
        
        if (InfoToStdOutput == "")
            InfoToStdOutput = "No mission is running at the background.\n";
        State = 0;
        return;
    }
    State = 0;
    for(int i = 1; i < ParaCount; ++i)
    {
        int WorkID;
        WorkID = atol(DividedCommand[i].c_str());

        if (WorkID == 0)
        {
            InfoToStdOutput = "Invalid ID.\n";
            continue;
        }
        else if (WorkID > Tail || WorkID <= Head || States[WorkID - 1] == 0)
        {
            InfoToStdOutput = "Can't find the target mission.\n";
            continue;
        }
        else if (States[WorkID - 1] == 1)
        {
            InfoToStdOutput = "Target is already running at the background.\n";
            continue;
        }
        else
        {
            States[WorkID - 1] = 1;
            kill(Jobs[WorkID - 1], SIGCONT);
        }
    }
    return;
}

void ExecCommandFg(string DividedCommand[], int ParaCount)
{
    if (ParaCount == 1)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Miss parameters.\n";
        State = 1;
    }
    else if (ParaCount > 2)
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else
    {
        int WorkID;
        WorkID = atol(DividedCommand[1].c_str());
        if (WorkID == 0)
        {
            InfoToStdOutput = "";
            InfoToStdError = "ERROR: Invalid ID.\n";
            State = 1;
        }
        else if (WorkID > Tail || WorkID <= Head || States[WorkID - 1] == 0)
        {
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
                write(TerminalOut, CMDofSons[WorkID - 1].c_str(), CMDofSons[WorkID - 1].length());
                write(TerminalOut, "\n", 1);
            }
            States[WorkID - 1] = 0;
            SonPID = Jobs[WorkID - 1];
            if (Tail == WorkID && Head == WorkID - 1)
                Tail = Head = 0;
            else if (Head == WorkID - 1)
                ++Head;
            else if (Tail == WorkID)
                --Tail;
            
            setpgid(SonPID, getgid());
            kill(SonPID, SIGCONT);
            while (SonPID != -1 && !waitpid(SonPID, NULL, WNOHANG));
            SonPID = -1;
        }
    }
}

void ExecCommandSet(string DividedCommand[], int ParaCount)
{
    extern char** environ;
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount == 1)
    {
        for (int i = 0; environ[i] != NULL; ++i)
            InfoToStdOutput = InfoToStdOutput + environ[i] + "\n";
    }
    else
    {
        InfoToStdOutput = "";
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    return;
}

void ExecCommandUmask(string DividedCommand[], int ParaCount)
{
    InfoToStdOutput = "";
    InfoToStdError = "";
    if (ParaCount > 2)
    {
        InfoToStdError = "ERROR: Too many parameters.\n";
        State = 1;
    }
    else if (ParaCount == 1)
    {
        mode_t CurrentMode = umask(0);
        umask(CurrentMode);
        InfoToStdOutput = ReturnUmaskValue(CurrentMode);
        State = 0;
    }
    else
    {
        if (DividedCommand[1].length() > 4)
        {
            InfoToStdError = "ERROR: At most 4 octonary.\n";
            State = 1;
        }
        else
        {
            int NewMode = GetOctValue(DividedCommand[1]);
            if (NewMode < 0)
            {
                InfoToStdError = "ERROR: Invalid umask value.\n";
                State = 1;
            }
            else
            {
                umask((mode_t)NewMode);
                State = 0;
            }
        }
    }
    return;
}