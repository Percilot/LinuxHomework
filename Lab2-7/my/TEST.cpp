#include <stdio.h>
#include <ctime>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <dirent.h>

#define BUF_SZ 256
#define TRUE 1
#define FALSE 0

const char *COMMAND_CD = "cd";
const char *COMMAND_EXIT = "exit";
const char *COMMAND_HELP = "help";
const char *COMMAND_PWD = "pwd";
const char *COMMAND_TIME = "time";
const char *COMMAND_CLR = "clr";
const char *COMMAND_DIR = "dir";
const char *COMMAND_SET = "set";
const char *COMMAND_ECHO = "echo";
const char *COMMAND_TEST = "test";
const char *COMMAND_IN = "<";
const char *COMMAND_OUT = ">";
const char *COMMAND_PIPE = "|";

// 内置的状态码
enum
{
    RESULT_NORMAL,
    ERROR_FORK,
    ERROR_COMMAND,
    ERROR_WRONG_PARAMETER,
    ERROR_MISS_PARAMETER,
    ERROR_TOO_MANY_PARAMETER,
    ERROR_CD,
    ERROR_SYSTEM,
    ERROR_EXIT,

    /* 重定向的错误信息 */
    ERROR_MANY_IN,
    ERROR_MANY_OUT,
    ERROR_FILE_NOT_EXIST,

    /* 管道的错误信息 */
    ERROR_PIPE,
    ERROR_PIPE_MISS_PARAMETER
};

char username[BUF_SZ];
char hostname[BUF_SZ];
char curPath[BUF_SZ];
char commands[BUF_SZ][BUF_SZ];

int GetCurDir();
void GetUserName();
void GetHostName();
int SplitCommands(char *command);
int ExecCommandCd(int CommandNum);
int ExecCommandPwd(int CommandNum);
int ExecCommandTime(int CommandNum);
int ExecCommandClr(int CommandNum);
int ExecCommandDir(int CommandNum);
int ExecCommandSet(int CommandNum);
int ExecCommandEcho(int CommandNum);
int ExecCommandTest(int CommandNum);
int ExecCommandExit();
void DealWithErrorSign(int ErrorSign);

int main()
{
    int Sign = GetCurDir();
    if (Sign == ERROR_SYSTEM)
    {
        fprintf(stderr, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
        exit(ERROR_SYSTEM);
    }
    GetUserName();
    GetHostName();

    char argv[BUF_SZ];
    while (1)
    {
        printf("\e[32;1m%s@%s:%s\e[0m$ ", username, hostname, curPath);
        fgets(argv, BUF_SZ, stdin);
        int Length = strlen(argv);
        if (Length != BUF_SZ)
            argv[Length - 1] = '\0';

        int CommandNum = SplitCommands(argv);

        if (CommandNum != 0)
        {
            if (strcmp(commands[0], COMMAND_CD) == 0)
            {
                Sign = ExecCommandCd(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
                else
                {
                    int DirSign = GetCurDir();
                    if (DirSign == ERROR_SYSTEM)
                    {
                        fprintf(stderr, "\e[31;1mError: System error while getting current work directory.\n\e[0m");
                        exit(ERROR_SYSTEM);
                    }
                }
            }

            else if (strcmp(commands[0], COMMAND_EXIT) == 0)
            {
                Sign = ExecCommandExit();
                if (Sign == ERROR_EXIT)
                    exit(-1);
            }

            else if (strcmp(commands[0], COMMAND_PWD) == 0)
            {
                Sign = ExecCommandPwd(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }

            else if (strcmp(commands[0], COMMAND_TIME) == 0)
            {
                Sign = ExecCommandTime(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }

            else if (strcmp(commands[0], COMMAND_CLR) == 0)
            {
                Sign = ExecCommandClr(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }

            else if (strcmp(commands[0], COMMAND_DIR) == 0)
            {
                Sign = ExecCommandDir(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }

            else if (strcmp(commands[0], COMMAND_ECHO) == 0)
            {
                Sign = ExecCommandEcho(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }

            else if (strcmp(commands[0], COMMAND_TEST) == 0)
            {
                Sign = ExecCommandTest(CommandNum);
                if (Sign != RESULT_NORMAL)
                    DealWithErrorSign(Sign);
            }
        }
    }
}

int GetCurDir()
{
    char *CurDir = getcwd(curPath, BUF_SZ);
    if (CurDir == NULL)
        return ERROR_SYSTEM;
    else
        return RESULT_NORMAL;
}

void GetUserName()
{
    struct passwd *Info = getpwuid(getuid());
    strcpy(username, Info->pw_name);
    return;
}

void GetHostName()
{
    gethostname(hostname, BUF_SZ);
    return;
}

int SplitCommands(char *command)
{
    int Count = 0;
    int i, j;
    int Length = strlen(command);
    memset(commands, 0, BUF_SZ * BUF_SZ);
    for (i = 0, j = 0; i < Length; ++i)
    {
        if (command[i] != ' ')
            commands[Count][j++] = command[i];
        else
        {
            if (j != 0)
            {
                commands[Count][j] = '\0';
                Count++;
                j = 0;
            }
        }
    }

    if (j != 0)
    {
        commands[Count][j] = '\0';
        Count++;
    }

    return Count;
}

int ExecCommandCd(int CommandNum)
{
    if (CommandNum < 2)
        return ERROR_MISS_PARAMETER;
    else if (CommandNum > 2)
        return ERROR_TOO_MANY_PARAMETER;
    else
    {
        int Sign = chdir(commands[1]);
        if (Sign)
            return ERROR_WRONG_PARAMETER;
        else
            return RESULT_NORMAL;
    }
}

int ExecCommandExit()
{
    pid_t pid = getpid();
    if (kill(pid, SIGTERM) == -1)
        return ERROR_EXIT;
    else
        return RESULT_NORMAL;
}

int ExecCommandPwd(int CommandNum)
{
    if (CommandNum > 1)
        return ERROR_TOO_MANY_PARAMETER;
    else
    {
        fprintf(stdout, "%s\n", curPath);
        return RESULT_NORMAL;
    }
}

int ExecCommandTime(int CommandNum)
{
    static const char *Week[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (CommandNum > 1)
        return ERROR_TOO_MANY_PARAMETER;
    else
    {
        time_t NowTime = time(NULL);
        struct tm *NowTimeStruct = localtime(&NowTime);
        fprintf(stdout, "%d.%d.%d ", NowTimeStruct->tm_year + 1900, NowTimeStruct->tm_mon + 1, NowTimeStruct->tm_mday);
        fprintf(stdout, "%s ", Week[NowTimeStruct->tm_wday]);
        fprintf(stdout, "%d:%d:%d\n", NowTimeStruct->tm_hour, NowTimeStruct->tm_min, NowTimeStruct->tm_sec);
        return RESULT_NORMAL;
    }
}

int ExecCommandClr(int CommandNum)
{
    if (CommandNum > 1)
        return ERROR_TOO_MANY_PARAMETER;
    else
    {
        system("clear");
        return RESULT_NORMAL;
    }
}

int ExecCommandDir(int CommandNum)
{
    if (CommandNum > 2)
        return ERROR_TOO_MANY_PARAMETER;
    else
    {
        if (CommandNum == 1)
            strcpy(commands[0], curPath);
        DIR *TargetDir;
        struct dirent *PointerToTarget;

        if (!(TargetDir = opendir(commands[0])))
        {
            fprintf(stderr, "\e[31;1mError: Target doesn't exist.\n\e[0m");
            return RESULT_NORMAL;
        }

        char Info[BUF_SZ][BUF_SZ];
        int Count = 0;
        while ((PointerToTarget = readdir(TargetDir)) != NULL)
            strcpy(Info[Count++], PointerToTarget->d_name);
        closedir(TargetDir);
        for (int i = 0; i < Count; ++i)
            fprintf(stdout, "%s\n", Info[i]);
        return RESULT_NORMAL;
    }
}

int ExecCommandEcho(int CommandNum)
{
    char Temp[BUF_SZ];
    memset(Temp, 0, BUF_SZ);
    for (int i = 1; i <= CommandNum; ++i)
    {
        strcat(Temp, commands[i]);
        strcat(Temp, " ");
    }
    strcat(Temp, "\n");
    fprintf(stdout, "%s", Temp);
    return RESULT_NORMAL;
}

int TimeCMP(const timespec& t1, const timespec& t2){
    if (t1.tv_sec < t2.tv_sec)
        return -1;
    if (t1.tv_sec > t2.tv_sec)
        return 1;
    if (t1.tv_nsec < t2.tv_nsec)
        return -1;
    if (t1.tv_nsec > t2.tv_nsec)
        return 1;
    return 0;
}

int ExecCommandTest(int CommandNum)
{
    int sign = 0;
    if (CommandNum < 3)
        return ERROR_MISS_PARAMETER;
    else if (CommandNum > 4)
        return ERROR_TOO_MANY_PARAMETER;
    else if (CommandNum == 3)
    {
        struct stat temp;
        if (strcmp(commands[1], "-e") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0)
                sign = 1;
        }
        else if (strcmp(commands[1], "-r") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && access(commands[1], R_OK))
                sign = 1;
        }
        else if (strcmp(commands[1], "-w") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && access(commands[1], W_OK))
                sign = 1;
        }
        else if (strcmp(commands[1], "-x") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && access(commands[1], X_OK))
                sign = 1;
        }
        else if (strcmp(commands[1], "-s") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && temp.st_size)
                sign = 1;
        }
        else if (strcmp(commands[1], "-d") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISDIR(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-f") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISREG(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-c") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISCHR(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-b") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISBLK(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-h") == 0 || strcmp(commands[1], "-L"))
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISLNK(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-p") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISFIFO(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-S") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && S_ISSOCK(temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-G") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && temp.st_gid == getgid())
                sign = 1;
        }
        else if (strcmp(commands[1], "-O") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && temp.st_uid == getuid())
                sign = 1;
        }
        else if (strcmp(commands[1], "-g") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && (S_ISGID & temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-u") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && (S_ISUID & temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-k") == 0)
        {
            int result = lstat(commands[2], &temp);
            if (result == 0 && (S_ISVTX & temp.st_mode))
                sign = 1;
        }
        else if (strcmp(commands[1], "-n") == 0)
        {
            if (strlen(commands[2]))
                sign = 1;
        }
        else if (strcmp(commands[1], "-z") == 0)
        {
            if (strlen(commands[2]) == 0)
                sign = 1;
        }
        else
            return ERROR_WRONG_PARAMETER;
    }
    else
    {
        struct stat Src1, Src2;
        if (strcmp(commands[1], "-ef") == 0)
        {
            int ret1 = lstat(commands[2], &Src1);
            int ret2 = lstat(commands[3], &Src2);
            if (ret1 == 0 && ret2 == 0 && Src1.st_dev == Src2.st_dev && Src1.st_ino == Src2.st_ino)
                sign = 1;
        }
        else if (strcmp(commands[1], "-nt") == 0)
        {
            int ret1 = lstat(commands[2], &Src1);
            int ret2 = lstat(commands[3], &Src2);
            if (ret1 == 0 && ret2 == 0 && TimeCMP(Src1.st_mtim, Src2.st_mtim) == 1)
                sign = 1;
        }
        else if (strcmp(commands[1], "-ot") == 0)
        {
            int ret1 = lstat(commands[2], &Src1);
            int ret2 = lstat(commands[3], &Src2);
            if (ret1 == 0 && ret2 == 0 && TimeCMP(Src1.st_mtim, Src2.st_mtim) == -1)
                sign = 1;
        }
        else
            return ERROR_WRONG_PARAMETER;
    }
    if (sign)
        fprintf(stdout, "True.\n");
    else
        fprintf(stdout, "Wrong.\n");
    return RESULT_NORMAL;
}

void DealWithErrorSign(int ErrorSign)
{
    switch (ErrorSign)
    {
    case ERROR_MISS_PARAMETER:
        fprintf(stderr, "\e[31;1mError: Miss parameter.\n\e[0m");
        break;
    case ERROR_TOO_MANY_PARAMETER:
        fprintf(stderr, "\e[31;1mError: Too many parameters.\n\e[0m");
        break;
    case ERROR_WRONG_PARAMETER:
        fprintf(stderr, "\e[31;1mError: Wrong parameters.\n\e[0m");
        break;
    default:
        break;
    }
}