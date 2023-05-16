#include "il2cpp-config.h"

#if !RUNTIME_TINY

#include "MemoryMappedFile.h"

#include "Baselib.h"
#include "Cpp/ReentrantLock.h"

#define BUFSZ 256
namespace il2cpp
{
namespace utils
{
    template <class T>
    inline void
    swap(T& i, T& j)
    {
        T tmp = i;
        i = j;
        j = tmp;
    }
    class RC4
    {
    public:
        void SetKey(char* K, int keylen);
        void Transform(unsigned char* output, const unsigned char* data, int len);

    private:
        unsigned char S[BUFSZ];
    };

    //初始化S盒
    void RC4::SetKey(char* K, int keylen)
    {
        for (int i = 0; i < BUFSZ; i++)
        {
            S[i] = i;
        }
        int j = 0;
        for (int i = 0; i < BUFSZ; i++)
        {
            j = (j + S[i] + K[i % keylen]) % BUFSZ;
            swap(S[i], S[j]);
        }
    }
    //生成密钥流
    void RC4::Transform(unsigned char* output, const unsigned char* data, int len)
    {
        int i = 0, j = 0;
        unsigned char key;
        for (int k = 0; k < len; k++)
        {
            i = (i + 1) % BUFSZ;
            j = (j + S[i]) % BUFSZ;
            swap(S[i], S[j]);
            key = S[(S[i] + S[j]) % BUFSZ];
            //按位异或操作
            output[k] = key ^ data[k];
        }
    }

    void* MemoryMappedFile::DecryptFile(char* data, int64_t length)
    {
        printf("length:%d", length);
        unsigned char* result;
        unsigned char* condata = (unsigned char*)data;
        // result = (char *)malloc(length);
        // char a[5] = "hxhz";
        // for (int i = 0; i < length; i++)
        // {
        //     result[i] = data[i] ^ a[i % 5];
        // }

        char K[] = "dbd7bd2873b977917658db924222d4cbd697";
        result = (unsigned char*)malloc(length);

        int keylen = sizeof(K);
        RC4 rc4encrypt, rc4decrypt;
        rc4encrypt.SetKey(K, keylen);
        rc4decrypt.SetKey(K, keylen);
        /* read/output BUFSZ bytes at a time */
        // while ((bytes = fread(buf, sizeof *buf, readsz, fp)) == readsz)
        // {
        //     // for (i = 0; i < readsz; i++)
        //     //     printf(" 0x%02x", buf[i]);

        //     rc4encrypt.Transform(buf, buf, BUFSZ);

        //     fwrite(buf, sizeof *buf, readsz, write_ptr);
        //     // putchar('\n');
        // }
        // for (int i = 0; i < length; i++)
        // {
        //     // result[i] = data[i] ^ a[i % 5];
        rc4decrypt.Transform(result, condata, length);
        // }
        // char *result = (char *)conresult;
        return result;
    }

    static baselib::ReentrantLock s_Mutex;
    static std::map<void*, os::FileHandle*> s_MappedAddressToMappedFileObject;
    static std::map<void*, int64_t> s_MappedAddressToMappedLength;

    void* MemoryMappedFile::Map(os::FileHandle* file)
    {
        return Map(file, 0, 0);
    }

    bool MemoryMappedFile::Unmap(void* address)
    {
        return Unmap(address, 0);
    }

    void* MemoryMappedFile::Map(os::FileHandle* file, int64_t length, int64_t offset)
    {
        return Map(file, length, offset, os::MMAP_FILE_ACCESS_READ);
    }

    void* MemoryMappedFile::Map(os::FileHandle* file, int64_t length, int64_t offset, int32_t access)
    {
        os::FastAutoLock lock(&s_Mutex);

        int64_t unused = 0;
        os::MemoryMappedFileError error = os::NO_MEMORY_MAPPED_FILE_ERROR;
        os::FileHandle* mappedFileHandle = os::MemoryMappedFile::Create(file, NULL, 0, &unused, (os::MemoryMappedFileAccess)access, 0, &error);
        if (error != 0)
            return NULL;

        int64_t actualOffset = offset;
        void* address = os::MemoryMappedFile::View(mappedFileHandle, &length, offset, (os::MemoryMappedFileAccess)access, &actualOffset, &error);

        if (address != NULL)
        {
            address = (uint8_t*)address + (offset - actualOffset);
            if (os::MemoryMappedFile::OwnsDuplicatedFileHandle(mappedFileHandle))
                s_MappedAddressToMappedFileObject[address] = mappedFileHandle;
            s_MappedAddressToMappedLength[address] = length;
        }

        return address;
    }

    bool MemoryMappedFile::Unmap(void* address, int64_t length)
    {
        os::FastAutoLock lock(&s_Mutex);

        if (length == 0)
        {
            std::map<void*, int64_t>::iterator entry = s_MappedAddressToMappedLength.find(address);
            if (entry != s_MappedAddressToMappedLength.end())
            {
                length = entry->second;
                s_MappedAddressToMappedLength.erase(entry);
            }
        }

        bool success = os::MemoryMappedFile::UnmapView(address, length);
        if (!success)
            return false;

        std::map<void*, os::FileHandle*>::iterator entry = s_MappedAddressToMappedFileObject.find(address);
        if (entry != s_MappedAddressToMappedFileObject.end())
        {
            bool result = os::MemoryMappedFile::Close(entry->second);
            s_MappedAddressToMappedFileObject.erase(entry);
            return result;
        }

        return true;
    }
}
}

#endif
