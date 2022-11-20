#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "synchconsole.h"
#include "machine.h"

#define MaxFileLength 32 // do dai toi da cua ten file
#define maxFile 20       // chi tao toi da 20 file descriptor

class fileDescriptorTable
{
private:
    int fType[maxFile];       // kieu file (-1: chua mo file, 0: doc va viet, 1: chi doc)
    OpenFile *fOpen[maxFile]; // mang luu file da mo, NULL neu chua mo file; id 0: console input va id 1: console output

    bool checkId(OpenFileId id)
    {
        if (0 <= id && id < maxFile) {
            if (fOpen[id] != NULL) {
                return true;
            }
            else {
                DEBUG(dbgSys, "\n OpenFileId has not opened \n");
            }
        }
        else {
            DEBUG(dbgSys, "\n Error OpenFileId \n");
        }
        return false;
    }

public:
    fileDescriptorTable()
    {

        fType[0] = 1;
        fOpen[0] = NULL;
        fType[1] = 0;
        fOpen[1] = NULL;

        for (OpenFileId i = 2; i < maxFile; i++)
        {
            fType[i] = -1;
            fOpen[i] = NULL;
        }
    }
    ~fileDescriptorTable()
    {
        for (OpenFileId i = 0; i < maxFile; i++)
        {
            if (fOpen[i] != NULL)
            {
                delete fOpen[i];
                fOpen[i] = NULL;
            }
        }
    }

public:
    OpenFileId Open(char *name, int type)
    {
        // neu type khac 0 va 1 thi bao sai
        if (type != 0 && type != 1)
        {
            DEBUG(dbgSys, "\n Error file type \n");
            return -1;
        }

        // kiem tra xem co file descriptor nao dang chong hay khong, neu co thi mo file luu vao file descriptor do
        for (OpenFileId i = 2; i < maxFile; i++)
        {
            if (fType[i] == -1)
            {
                fOpen[i] = kernel->fileSystem->Open(name);
                if (fOpen[i] != NULL)
                {
                    fType[i] = type;
                    return i;
                }
                DEBUG(dbgSys, "\n Can not open file \n");
                return -1;
            }
        }

        DEBUG(dbgSys, "\n File descriptor table is full \n");
        return -1;
    }

    int Close(OpenFileId id)
    {
        // file descriptor 0 va 1 la console input và console output nen khong duoc dong
        if (id == 0 || id == 1)
        {
            
            DEBUG(dbgSys, "\n Can not close console input or console output \n");
            return -1;
        }
        if (checkId(id))
        {
            delete fOpen[id];
            fOpen[id] = NULL;
            fType[id] = -1;
            return 0;
        }
        return -1;
    }

    int Read(char *buffer, int size, OpenFileId id)
    {
        if (size < 0)
        {
            DEBUG(dbgSys, "\n Size can not be negative integer \n");
            return -1;
        }

        // SynchConsoleInput
        if (id == 0)
        {
            int i;
            buffer[size] = '\0';
            for (i = 0; i < size; i++)
            {
                buffer[i] = kernel->synchConsoleIn->GetChar();
                if (buffer[i] == '\n')
                { // nhap den khi xuong dong
                    break;
                }
            }
            if (buffer[i] != '\n')
                while (kernel->synchConsoleIn->GetChar() != '\n'); // xoa cac ki tu con du trong
            buffer[i] = '\0';
            return i;
        }

        // SynchConsoleOutput
        // khong cho phep doc
        if (id == 1)
        {
            DEBUG(dbgSys, "\n Can not read at console output \n");
            return -1;
        }

        if (checkId(id))
        {
            if (fType[id] == 0 || fType[id] == 1)
            {
                return fOpen[id]->Read(buffer, size);
            }
        }
        return -1;
    }

    int Write(char *buffer, int size, OpenFileId id)
    {
        if (size < 0)
        {
            DEBUG(dbgSys, "\n Size can not be negative integer \n");
            return -1;
        }

        // SynchConsoleInput
        // khong cho phep viet
        if (id == 0)
        {
            DEBUG(dbgSys, "\n Can not write at console input \n");
            return -1;
        }

        // SynchConsoleOutput
        if (id == 1)
        {
            int i;
            for (i = 0; i < size; i++)
            {
                if (buffer[i] == '\0')
                { // het chuoi
                    break;
                }
                kernel->synchConsoleOut->PutChar(buffer[i]);
            }
            return i;
        }

        if (checkId(id))
        {
            if (fType[id] == 0)
            {
                return fOpen[id]->Write(buffer, size);
            }
            else {
                DEBUG(dbgSys, "\n Can not write at only read file \n");
            }
        }
        return -1;
    }

    int Seek(int position, OpenFileId id)
    {
        // goi seek tren SynchConsole bao loi
        if (id == 0 || id == 1)
        {
            DEBUG(dbgSys, "\n Can not seek at console input or console output \n");
            return -1;
        }

        if (checkId(id))
        {
            int maxLen = fOpen[id]->Length();

            // neu position = -1 thi chuyen ve cuoi file
            if (position == -1)
            {
                position = maxLen;
            }
            // neu position < -1 hoac vuoc qua kich thuong file bao loi
            else if (position < -1 || position > maxLen)
            {
                DEBUG(dbgSys, "\n Error position \n");
                return -1;
            }

            fOpen[id]->Seek(position);
            return position;
        }
        return -1;
    }
};
