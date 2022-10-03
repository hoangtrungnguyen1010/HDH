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
void increasePC(){
        /* set previous programm counter (debugging only)*/
    kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

    /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
    kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

    /* set next programm counter for brach execution */
    kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);

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

void SysCall_PrintNum()
{
    int n = kernel->machine->ReadRegister(4);
    /*int: [-2147483648 , 2147483647] --> max length = 11*/
    const int maxlen = 11;
    char num_string[maxlen] = {0};
    int tmp[maxlen] = {0}, i = 0, j = 0;
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

unsigned int SysCall_Random()
{
    unsigned int num = RandomNumber();
    increasePC();
    return num;
}
void ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception test" << which << " type: " << type << "\n");

    switch (which)
    {
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
