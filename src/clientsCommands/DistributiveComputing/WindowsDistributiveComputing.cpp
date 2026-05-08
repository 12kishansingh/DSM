#ifdef _WIN32

#include "../../../headers/clientsCommand.hpp"
#include "../../../headers/shareFile.hpp"
#include "../../../headers/threadSafety.hpp"
#include "../../../headers/Status_codes.hpp"

#include <windows.h>
#include <io.h>
#include <fcntl.h>

#include <atomic>
#include <future>
#include <thread>
#include <string>
#include <iostream>

off_t *train_offset;
int_fast64_t *train_chunk_size;

// ------------------------------------------------------------
// UTIL
// ------------------------------------------------------------

int findTotalConnections()
{
    int connections = 0;

    while (ip_list[connections] != NULL)
        connections++;

    return connections;
}

void freeMemory()
{
    for (int i = 0; ip_list[i] != NULL; i++)
    {
        free(ip_list[i]);
        free(ip_status[i]);
    }

    free(ip_list);
    free(ip_status);

    free(train_offset);
    free(train_chunk_size);
}

int_fast64_t getFileSize(const char *filePath)
{
    HANDLE hFile = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open file\n");
        return -1;
    }

    LARGE_INTEGER size;

    if (!GetFileSizeEx(hFile, &size))
    {
        CloseHandle(hFile);
        printf("Failed to get file size\n");
        return -1;
    }

    CloseHandle(hFile);

    return size.QuadPart;
}

// ------------------------------------------------------------
// PREAD EMULATION
// ------------------------------------------------------------

ssize_t pread_windows(HANDLE hFile,
                      void *buffer,
                      size_t size,
                      int64_t offset)
{
    OVERLAPPED ov = {};

    ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);

    DWORD bytesRead = 0;

    BOOL ok = ReadFile(
        hFile,
        buffer,
        (DWORD)size,
        &bytesRead,
        &ov);

    if (!ok)
        return -1;

    return bytesRead;
}

// ------------------------------------------------------------
// SAFE LINE ALIGNMENT
// ------------------------------------------------------------

int_fast64_t adjustToNextLine(HANDLE hFile,
                              int_fast64_t pos,
                              int_fast64_t fileSize)
{
    if (pos >= fileSize)
        return fileSize;

    const size_t BUF_SIZE = 4096;

    char buffer[BUF_SIZE];

    int_fast64_t start = pos;

    while (pos < fileSize &&
           pos - start < BUF_SIZE * 4)
    {
        ssize_t bytesRead =
            pread_windows(hFile,
                          buffer,
                          BUF_SIZE,
                          pos);

        if (bytesRead <= 0)
            break;

        for (ssize_t i = 0; i < bytesRead; i++)
        {
            if (buffer[i] == '\n')
                return pos + i + 1;
        }

        pos += bytesRead;
    }

    return start;
}

// ------------------------------------------------------------
// FILE DIVISION
// ------------------------------------------------------------

void divideFileIntoChunks(const char *trainfilepath,
                          int totalConnections)
{
    HANDLE hFile = CreateFileA(
        trainfilepath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("File open failed\n");
        return;
    }

    int_fast64_t fileSize =
        getFileSize(trainfilepath);

    if (fileSize <= 0)
    {
        printf("Invalid file size\n");
        CloseHandle(hFile);
        return;
    }

    int totalWeight = 0;

    for (int i = 0; i < totalConnections; i++)
        totalWeight += (i == 0) ? 3 : 1;

    int_fast64_t offset = 0;

    for (int i = 0; i < totalConnections; i++)
    {
        int currWeight = (i == 0) ? 3 : 1;

        int_fast64_t ideal_chunk =
            (fileSize * currWeight) / totalWeight;

        int_fast64_t tentative_end =
            offset + ideal_chunk;

        int_fast64_t adjusted_end;

        if (i == totalConnections - 1)
        {
            adjusted_end = fileSize;
        }
        else
        {
            adjusted_end =
                adjustToNextLine(
                    hFile,
                    tentative_end,
                    fileSize);

            if (adjusted_end <= offset ||
                adjusted_end >
                    tentative_end + ideal_chunk)
            {
                adjusted_end = tentative_end;
            }
        }

        int_fast64_t final_chunk =
            adjusted_end - offset;

        train_offset[i] = offset;
        train_chunk_size[i] = final_chunk;

        offset = adjusted_end;
    }

    CloseHandle(hFile);
}

// ------------------------------------------------------------
// SEND FILE THREAD
// ------------------------------------------------------------

void *send_file_thread(send_file_args fargs)
{
    int result;

    if (fargs.use_chunk)
    {
        result = send_file(
            fargs.filePath,
            fargs.IP,
            fargs.tag,
            fargs.iscmdSendFile,
            fargs.offset,
            fargs.chunk_size);
    }
    else
    {
        result = send_file(
            fargs.filePath,
            fargs.IP,
            fargs.tag,
            fargs.iscmdSendFile);
    }

    if (result < 0)
    {
        fprintf(stderr,
                "Failed to send %s file to %s\n",
                fargs.tag,
                fargs.IP);
    }

    return NULL;
}

// ------------------------------------------------------------
// LOCAL DISTRIBUTIVE COMPUTING
// ------------------------------------------------------------

long long distributiveComputingLocal(void *args)
{
    auto *dis_args =
        (distributiveComputingargs *)args;

    printf("Local → Offset: %lld, Size: %lld\n",
           dis_args->train_offset,
           dis_args->train_chunk_size);

    static const char *binaryPath =
        "C:\\Temp\\local_exec.exe";

    // --------------------------------------------------------
    // COMPILE ONCE
    // --------------------------------------------------------

    static std::once_flag compile_flag;

    std::call_once(
        compile_flag,
        [&]()
        {
            std::string cmd =
                "g++ " +
                std::string(dis_args->codePath) +
                " -O2 -o " +
                std::string(binaryPath);

            STARTUPINFOA si = {};
            PROCESS_INFORMATION pi = {};

            si.cb = sizeof(si);

            BOOL ok = CreateProcessA(
                NULL,
                cmd.data(),
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &si,
                &pi);

            if (!ok)
            {
                printf("Compilation process failed\n");
                exit(1);
            }

            WaitForSingleObject(
                pi.hProcess,
                INFINITE);

            DWORD exitCode;

            GetExitCodeProcess(
                pi.hProcess,
                &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode != 0)
            {
                printf("Compilation failed\n");
                exit(1);
            }
        });

    // --------------------------------------------------------
    // PIPES
    // --------------------------------------------------------

    HANDLE childStdoutRead;
    HANDLE childStdoutWrite;

    HANDLE childStdinRead;
    HANDLE childStdinWrite;

    SECURITY_ATTRIBUTES sa = {};

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    CreatePipe(
        &childStdoutRead,
        &childStdoutWrite,
        &sa,
        0);

    CreatePipe(
        &childStdinRead,
        &childStdinWrite,
        &sa,
        0);

    SetHandleInformation(
        childStdoutRead,
        HANDLE_FLAG_INHERIT,
        0);

    SetHandleInformation(
        childStdinWrite,
        HANDLE_FLAG_INHERIT,
        0);

    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};

    si.cb = sizeof(si);

    si.dwFlags |= STARTF_USESTDHANDLES;

    si.hStdInput = childStdinRead;
    si.hStdOutput = childStdoutWrite;
    si.hStdError = childStdoutWrite;

    BOOL ok = CreateProcessA(
        binaryPath,
        NULL,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi);

    if (!ok)
    {
        printf("Failed to start child process\n");
        return -1;
    }

    CloseHandle(childStdoutWrite);
    CloseHandle(childStdinRead);

    // --------------------------------------------------------
    // SEND DATA TO CHILD
    // --------------------------------------------------------

    HANDLE hFile = CreateFileA(
        dis_args->trainFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open train file\n");
        return -1;
    }

    const size_t BUF_SIZE = 65536;

    char buffer[BUF_SIZE];

    int64_t remaining =
        dis_args->train_chunk_size;

    int64_t offset =
        dis_args->train_offset;

    while (remaining > 0)
    {
        ssize_t to_read =
            std::min(
                (int64_t)BUF_SIZE,
                remaining);

        ssize_t bytesRead =
            pread_windows(
                hFile,
                buffer,
                to_read,
                offset);

        if (bytesRead <= 0)
        {
            printf("Read failed\n");
            break;
        }

        DWORD totalWritten = 0;

        WriteFile(
            childStdinWrite,
            buffer,
            bytesRead,
            &totalWritten,
            NULL);

        offset += bytesRead;
        remaining -= bytesRead;
    }

    CloseHandle(hFile);

    CloseHandle(childStdinWrite);

    // --------------------------------------------------------
    // READ OUTPUT
    // --------------------------------------------------------

    std::string output;

    char outbuf[256];

    DWORD bytesRead = 0;

    while (true)
    {
        BOOL success =
            ReadFile(
                childStdoutRead,
                outbuf,
                sizeof(outbuf),
                &bytesRead,
                NULL);

        if (!success || bytesRead == 0)
            break;

        output.append(outbuf, bytesRead);
    }

    CloseHandle(childStdoutRead);

    WaitForSingleObject(
        pi.hProcess,
        INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    long long result = 0;

    try
    {
        result = std::stoll(output);
    }
    catch (...)
    {
        printf("Invalid output\n");
        return -1;
    }

    printf("Local computation result: %lld\n",
           result);

    return result;
}

#endif