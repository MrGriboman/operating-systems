#include <bg_start.h>

#ifdef _WIN32
int create_bg_process(const char *program, char *const argv[], int do_wait)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    LPSTR command = program;

    if (!CreateProcess(
            NULL,
            command,
            NULL,
            NULL,
            0,
            0,
            NULL,
            NULL,
            &si,
            &pi))
    {
        printf("CreateProcess failed (%lu)\n", GetLastError());
        return 1;
    }

    if (do_wait)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode))
        {
            printf("Process exited with code %lu\n", exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode;
        }
        else
        {
            printf("Failed to get exit code (%lu)", GetLastError());
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    Sleep(10);
    return 0;
}

#else
void handle_sigchld(int sig)
{
    int status;
    pid_t pid;

    // Use WNOHANG to reap all terminated child processes without blocking
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
    }
}

int create_bg_process(const char *program, char *const argv[], int do_wait)
{
    pid_t pid;
    switch (pid = fork())
    {
    case -1:
        perror("fork has failed");
        break;

    case 0:
        printf("CHILD: My ID is %d, my parent's ID is %d\n", getpid(), getppid());
        execvp(program, argv);
        perror("exec failed");
        exit(1);
        break;

    default:
        int status;
        if (do_wait)
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("waitpid failed");
            }

            if (WIFEXITED(status))
            {
                printf("Child finished with the exit code %d\n", WEXITSTATUS(status));
            }

            else
            {
                printf("Child has not finished correctly");
            }
        }
        else
        {
            struct sigaction sa;
            sa.sa_handler = handle_sigchld;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;

            if (sigaction(SIGCHLD, &sa, NULL) == -1)
            {
                perror("sigaction failed");
                return 1;
            }
        }
        printf("PARENT: My ID is %d\n", getpid());
        int sec = 0;
        while ((sec = sleep(sec)) > 0)
        {
            sleep(sec);
        }
        break;
    }
    return 0;
}
#endif

int main()
{
    const char *program = "./util.exe";
    char *const args[] = {NULL};
    create_bg_process(program, args, 0);
    return 0;
}
