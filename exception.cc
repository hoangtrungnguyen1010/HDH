// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "synchconsole.h"
#include "machine.h"
#include "fileDescriptorTable.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

#define MaxFileLength 32 // do dai toi da cua ten file
#define maxFile 20       // chi tao toi da 20 file descriptor
fileDescriptorTable FileDescriptorTable;

void increasePC()
{
    /* set previous programm counter (debugging only)*/
    kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

    /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
    kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

    /* set next programm counter for brach execution */
    kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

/*
Input: - User space address (int)
 - Limit of buffer (int)
Output:- Buffer (char*)
Purpose: Copy buffer from User memory space to System memory space
*/
char *User2System(int virtAddr, int limit)
{
    int i; // index
    int oneChar;
    char *kernelBuf = NULL;

    kernelBuf = new char[limit + 1]; // need for terminal string
    if (kernelBuf == NULL)
        return kernelBuf;
    memset(kernelBuf, 0, limit + 1);

    // printf("\n Filename u2s:");
    for (i = 0; i < limit; i++)
    {
        kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
        kernelBuf[i] = (char)oneChar;
        // printf("%c",kernelBuf[i]);
        if (oneChar == 0)
            break;
    }
    kernelBuf[limit] = '\0';
    return kernelBuf;
}

/*
Input: - User space address (int)
 - Limit of buffer (int)
 - Buffer (char[])
Output:- Number of bytes copied (int)
Purpose: Copy buffer from System memory space to User memory space
*/
int System2User(int virtAddr, int len, char *buffer)
{
    if (buffer == NULL)
        return -1;
    if (len < 0)
        return -1;
    if (len == 0)
        return len;
    int i = 0;
    int oneChar = 0;
    do
    {
        oneChar = (int)buffer[i];
        kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
        i++;
    } while (i < len && oneChar != 0);
    return i;
}

void SysCall_Open()
{
    int virtAddr;
    char *filename;
    virtAddr = kernel->machine->ReadRegister(4);
    filename = User2System(virtAddr, MaxFileLength + 1);
    int type = kernel->machine->ReadRegister(5);

    // khong lay duoc file name
    if (filename == NULL)
    {
        DEBUG(dbgSys, "\n Not enough memory in system \n");
        kernel->machine->WriteRegister(2, -1);
        increasePC();
        return;
    }

    int result = FileDescriptorTable.Open(filename, type);
    if (result == -1) {
        DEBUG(dbgSys, "\n Can not open file \n");
    }
    kernel->machine->WriteRegister(2, result);

    delete[] filename;
    increasePC();
    return;
}

void SysCall_Close()
{
    OpenFileId id = kernel->machine->ReadRegister(4);
    int result = FileDescriptorTable.Close(id);
    if (result == -1) {
        DEBUG(dbgSys, "\n Can not close file \n");
    }
    kernel->machine->WriteRegister(2, result);
    increasePC();
    return;
}

void SysCall_Remove()
{
    FileSystem *s;
    int virtAddr = kernel->machine->ReadRegister(4);
    char *fileName = User2System(virtAddr, MaxFileLength + 1);
    //Khong du bo nho o system
    if (fileName == NULL)
    {
        DEBUG(dbgSys, "\n Not enough memory in system \n");
        kernel->machine->WriteRegister(2, -1);
    }

    else
    {
        //Xoa file thanh cong
        if (s->Remove(fileName))
        {
            kernel->machine->WriteRegister(2, 0);
        }
        //Xoa file that bai
        else
        {
            DEBUG(dbgSys, "\n Can not remove file \n");
            kernel->machine->WriteRegister(2, -1);
        }
    }

    delete[] fileName;
    increasePC();
}

void SysCall_Create()
{
    FileSystem *s;
    int virtAddr = kernel->machine->ReadRegister(4);
    char *fileName = User2System(virtAddr, MaxFileLength + 1);
    // Truong hop khong du bo nho
    if (fileName == NULL)
    {
        DEBUG(dbgSys, "\n Not enough memory in system \n");
        kernel->machine->WriteRegister(2, -1);
    }
    else
    {
        //Tao file khong thanh cong
        if (s->Create(fileName) == 0)
        {
            DEBUG(dbgSys, "\n Can not create file \n");
            kernel->machine->WriteRegister(2, -1);
        }
        //Tao file thanh cong
        else
        {
            kernel->machine->WriteRegister(2, 0);
        }
    }
    delete[] fileName;
    increasePC();
}

void SysCall_Read()
{
    int virtAddr = kernel->machine->ReadRegister(4);
    int size = kernel->machine->ReadRegister(5);
    OpenFileId id = kernel->machine->ReadRegister(6);

    char *buffer = new char[size + 1];
    // khong cap phat duoc vung nho cho buffer
    if (buffer == NULL)
    {
        DEBUG(dbgSys, "\n Not enough memory in system \n");
        kernel->machine->WriteRegister(2, -1);
        increasePC();
        return;
    }

    int result = FileDescriptorTable.Read(buffer, size, id);
    kernel->machine->WriteRegister(2, result);

    if (result != -1)
    {
        System2User(virtAddr, size, buffer);
    }
    else {
        DEBUG(dbgSys, "\n Can not read file \n");
    }

    delete[] buffer;
    increasePC();
    return;
}

void SysCall_Write()
{
    int virtAddr = kernel->machine->ReadRegister(4);
    int size = kernel->machine->ReadRegister(5);
    OpenFileId id = kernel->machine->ReadRegister(6);

    char *buffer = User2System(virtAddr, size);
    // khong sao chep duoc chuoi
    if (buffer == NULL)
    {
        DEBUG(dbgSys, "\n Not enough memory in system \n");
        kernel->machine->WriteRegister(2, -1);
        increasePC();
        return;
    }

    int result = FileDescriptorTable.Write(buffer, size, id);
    if (result == -1) {
        DEBUG(dbgSys, "\n Can not write file \n");
    }
    kernel->machine->WriteRegister(2, result);

    delete[] buffer;
    increasePC();
    return;
}

void SysCall_Seek()
{
    int position = kernel->machine->ReadRegister(4);
    OpenFileId id = kernel->machine->ReadRegister(5);
    
    int result = FileDescriptorTable.Seek(position, id);
    if (result == -1) {
        DEBUG(dbgSys, "\n Can not seek file \n");
    }
    kernel->machine->WriteRegister(2, result);
    increasePC();
    return;
}

void SysCall_ReadNum()
{
    char c;

    // loai bo ki tu trang o dau
    do
    {
        c = kernel->synchConsoleIn->GetChar();
    } while (c == ' ' || c == '\n');

    // kiem tra dau cua so
    bool checkMinus = false;

    // check == 0: trang thai hien tai bang gioi han cua so
    // check == -1: trang thai hien tai be hon gioi han cua so
    // check == 1: trang thai hien tai lon hon gioi han cua so
    // check == 2: khong phai so
    // check == 3: tran so
    int check = 0;

    if (c == '+')
    {
        c = kernel->synchConsoleIn->GetChar();
        if (!('0' <= c && c <= '9'))
        {
            check = 2;
        }
    }
    else if (c == '-')
    {
        c = kernel->synchConsoleIn->GetChar();
        checkMinus = true;
        if (!('0' <= c && c <= '9'))
        {
            check = 2;
        }
    }
    // loai bo so 0 o dau
    while (c == '0')
    {
        c = kernel->synchConsoleIn->GetChar();
    }
    // Khoảng của int [-2147483648 , 2147483647]

    char stringNum[12] = "2147483648";
    if (!checkMinus)
    {
        stringNum[9] = '7';
    }

    int i = 0;
    while (c != ' ' && c != '\n')
    {
        if (check < 2)
        {
            if ('0' <= c && c <= '9')
            {
                if (stringNum[i] != '\0')
                {

                    // cap nhat lai trang thai stringNum so voi gioi han so
                    if (check == 0)
                    {
                        if (stringNum[i] < c)
                        {
                            check = 1;
                        }
                        else if (stringNum[i] > c)
                        {
                            check = -1;
                        }
                    }

                    // cap nhat lai stringNum
                    stringNum[i] = c;
                    i++;
                }
                // tran so
                else
                    check = 3;
            }
            // khong phai so
            else
                check = 2;
        }
        c = kernel->synchConsoleIn->GetChar();
    }
    // tran so
    if (check == 1 && stringNum[i] == '\0')
        check = 3;
    else if (check < 2)
        stringNum[i] = '\0';

    if (check == 3)
    {
        if (checkMinus)
        {
            kernel->machine->WriteRegister(2, -2147483648);
        }
        else
        {
            kernel->machine->WriteRegister(2, 2147483647);
        }
        increasePC();
        return;
    }

    if (check == 2)
    {
        stringNum[0] = '\0';
    }

    // ghi ket qua
    int result = 0;
    for (i = 0; stringNum[i] != '\0'; i++)
    {
        result = result * 10 - int(stringNum[i] - '0');
    }
    if (!checkMinus)
        result = -result;
    kernel->machine->WriteRegister(2, result);

    increasePC();
}

void SysCall_PrintNum()
{
    //Ý tưởng
    int n = kernel->machine->ReadRegister(4);
    // Khoảng của int [-2147483648 , 2147483647]
    const int length = 11;
    char num_string[length] = {0};
    int tmp[length] = {0}, i = 0, j = 0;
    bool check = 0;

    if (n < 0)
    {
        if (n == -2147483648)
        {
            check = 1;
            n += 1;
        }

        n = -n;
        num_string[i++] = '-';
    }

    do
    {
        tmp[j++] = n % 10;
        n /= 10;
    } while (n);

    while (j)
        num_string[i++] = '0' + (char)tmp[--j];
    if (check == 1)
    {
        num_string[--i] += 1;
        i++;
    }

    SynchConsoleOutput *console = new SynchConsoleOutput(NULL);
    int size = i;
    i = 0;
    while (i < size)
    {
        console->PutChar(num_string[i++]);
    }
    increasePC();
}

char SysCall_ReadChar()
{
    // SynchConsoleInput *console = new SynchConsoleInput(NULL);
    // char c = console->GetChar();

    char c = kernel->synchConsoleIn->GetChar();
    kernel->machine->WriteRegister(2, c);

    increasePC();
    return c;
}

void Syscall_PrintChar()
{
    char n = kernel->machine->ReadRegister(4);
    SynchConsoleOutput *console = new SynchConsoleOutput(NULL);
    console->PutChar(n);

    increasePC();
}

unsigned int SysCall_Random()
{
    unsigned int num = RandomNumber();
    increasePC();
    return num;
}

void SysCall_ReadString()
{
    // virAddr chua dia chi chuoi de luu vao
    // len la do dai toi da cua chuoi
    int virtAddr = kernel->machine->ReadRegister(4);
    int len = kernel->machine->ReadRegister(5), i;

    char oneChar;
    // xoa cac phan tu xuong dong trong synchConsoleIn truoc khi nhap chuoi
    do
    {
        oneChar = kernel->synchConsoleIn->GetChar();
    } while (oneChar == '\n');

    for (i = 0; i < len; i++)
    {

        // them phan tu vao chuoi
        kernel->machine->WriteMem((virtAddr + i), 1, oneChar);

        // doc mot phan tu trong synchConsoleIn
        oneChar = kernel->synchConsoleIn->GetChar();

        // kiem tra nguoi dung nhap het chuoi
        if (oneChar == '\n')
        {
            break;
        }
    }

    // xoa cac phan tu thua trong synchConsoleIn
    if (oneChar != '\n')
        while (kernel->synchConsoleIn->GetChar() != '\n')
            ;

    // them ky tu ket thuc chuoi
    kernel->machine->WriteMem((virtAddr + i + 1), 1, '\0');

    increasePC();
}

void SysCall_PrintString()
{
    int i = 0;
    int oneChar;
    // lay dia chi chuoi
    int virtAddr = kernel->machine->ReadRegister(4);

    while (true)
    {
        // lay mot phan tu trong chuoi
        kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
        // in phan tu do len synchConsoleOut
        kernel->synchConsoleOut->PutChar((char)oneChar);
        i++;

        // kiem tra ket thuc chuoi
        if (oneChar == '\0')
        {
            break;
        }
    }

    increasePC();
}

void ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception  " << which << " type: " << type << "\n");

    switch (which)
    {
        // Trả quyền điều khiển cho hệ điều hành
    case NoException:
        break;
        // Với những exception khác thông báo lỗi và halt hệ thống
    case PageFaultException:
        DEBUG(dbgSys, "PageFaultException - No valid translation found\n");
        kernel->interrupt->Halt();

        break;
    case ReadOnlyException:
        DEBUG(dbgSys, "ReadOnlyException -  Write attempted to page marked	 \"read-only\" \n");
        kernel->interrupt->Halt();

        break;
    case BusErrorException:
        DEBUG(dbgSys, "BusErrorException - Translation resulted in an invalid physical address\n");
        kernel->interrupt->Halt();

        break;
    case AddressErrorException:
        DEBUG(dbgSys, "AddressErrorException - Unaligned reference or one that was beyond the end of the address space\n");
        kernel->interrupt->Halt();

        break;
    case OverflowException:
        DEBUG(dbgSys, "OverflowException - Integer overflow in add or sub\n");
        kernel->interrupt->Halt();

        break;
    case IllegalInstrException:
        DEBUG(dbgSys, "IllegalInstrException - Unimplemented or reserved instr\n");
        kernel->interrupt->Halt();

        break;
    case NumExceptionTypes:
        DEBUG(dbgSys, "Unexpected user mode exception NumExceptionTypes\n");
        kernel->interrupt->Halt();

        break;

    case SyscallException:
        DEBUG(dbgSys, "Receive syscall exception.\n");

        switch (type)
        {
        case SC_Halt:
            DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

            SysHalt();

            ASSERTNOTREACHED();
            break;

        case SC_Add:
            DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

            /* Process SysAdd Systemcall*/
            int result;
            result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
                            /* int op2 */ (int)kernel->machine->ReadRegister(5));

            DEBUG(dbgSys, "Add returning with " << result << "\n");
            /* Prepare Result */
            kernel->machine->WriteRegister(2, (int)result);
            increasePC();
            return;

            ASSERTNOTREACHED();

            break;

        case SC_Open:
            SysCall_Open();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Close:
            SysCall_Close();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Read:
            SysCall_Read();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Write:
            SysCall_Write();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Seek:
            SysCall_Seek();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Create:
            SysCall_Create();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Remove:
            SysCall_Remove();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_ReadNum:
            SysCall_ReadNum();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_PrintNum:
            SysCall_PrintNum();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_ReadChar:
            SysCall_ReadChar();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_PrintChar:
            Syscall_PrintChar();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_Random:
            SysCall_Random();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_ReadString:
            SysCall_ReadString();
            return;
            ASSERTNOTREACHED();
            break;

        case SC_PrintString:
            SysCall_PrintString();
            return;
            ASSERTNOTREACHED();
            break;

        default:
            cerr << "Unexpected system call  " << type << "\n";
            break;
        }
        break;
    default:
        cerr << "Unexpected user mode exception" << (int)which << "\n";
        break;
    }
    ASSERTNOTREACHED();
}
