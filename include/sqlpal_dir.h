#if !defined(DISABLE_LIST_DIR_NAME) && defined(_WIN32)

#ifndef SQLPAL_DIR_H
#define SQLPAL_DIR_H

#define UNICODE

#include <windows.h>
#include <strsafe.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_SUCCESS                       0x00000000
#define STATUS_NO_MORE_FILES                 0x80000006

#define OBJ_CASE_INSENSITIVE                 0x00000040

#define FILE_DIRECTORY_FILE                  0x00000001
#define FILE_WRITE_THROUGH                   0x00000002
#define FILE_SEQUENTIAL_ONLY                 0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING       0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT            0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT         0x00000020
#define FILE_NON_DIRECTORY_FILE              0x00000040
#define FILE_CREATE_TREE_CONNECTION          0x00000080
#define FILE_COMPLETE_IF_OPLOCKED            0x00000100
#define FILE_NO_EA_KNOWLEDGE                 0x00000200
#define FILE_OPEN_REMOTE_INSTANCE            0x00000400
#define FILE_RANDOM_ACCESS                   0x00000800
#define FILE_DELETE_ON_CLOSE                 0x00001000
#define FILE_OPEN_BY_FILE_ID                 0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT          0x00004000
#define FILE_NO_COMPRESSION                  0x00008000
#define FILE_RESERVE_OPFILTER                0x00100000
#define FILE_OPEN_REPARSE_POINT              0x00200000
#define FILE_OPEN_NO_RECALL                  0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY       0x00800000
#define FILE_COPY_STRUCTURED_STORAGE         0x00000041
#define FILE_STRUCTURED_STORAGE              0x00000441

typedef LONG NTSTATUS;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID    Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID (NTAPI *PIO_APC_ROUTINE) (
  IN PVOID ApcContext,
  IN PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG Reserved
);

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation                  = 1,
    FileFullDirectoryInformation,            // 2
    FileBothDirectoryInformation,            // 3
    FileBasicInformation,                    // 4
    FileStandardInformation,                 // 5
    FileInternalInformation,                 // 6
    FileEaInformation,                       // 7
    FileAccessInformation,                   // 8
    FileNameInformation,                     // 9
    FileRenameInformation,                   // 10
    FileLinkInformation,                     // 11
    FileNamesInformation,                    // 12
    FileDispositionInformation,              // 13
    FilePositionInformation,                 // 14
    FileFullEaInformation,                   // 15
    FileModeInformation,                     // 16
    FileAlignmentInformation,                // 17
    FileAllInformation,                      // 18
    FileAllocationInformation,               // 19
    FileEndOfFileInformation,                // 20
    FileAlternateNameInformation,            // 21
    FileStreamInformation,                   // 22
    FilePipeInformation,                     // 23
    FilePipeLocalInformation,                // 24
    FilePipeRemoteInformation,               // 25
    FileMailslotQueryInformation,            // 26
    FileMailslotSetInformation,              // 27
    FileCompressionInformation,              // 28
    FileObjectIdInformation,                 // 29
    FileCompletionInformation,               // 30
    FileMoveClusterInformation,              // 31
    FileQuotaInformation,                    // 32
    FileReparsePointInformation,             // 33
    FileNetworkOpenInformation,              // 34
    FileAttributeTagInformation,             // 35
    FileTrackingInformation,                 // 36
    FileIdBothDirectoryInformation,          // 37
    FileIdFullDirectoryInformation,          // 38
    FileValidDataLengthInformation,          // 39
    FileShortNameInformation,                // 40
    FileIoCompletionNotificationInformation, // 41
    FileIoStatusBlockRangeInformation,       // 42
    FileIoPriorityHintInformation,           // 43
    FileSfioReserveInformation,              // 44
    FileSfioVolumeInformation,               // 45
    FileHardLinkInformation,                 // 46
    FileProcessIdsUsingFileInformation,      // 47
    FileNormalizedNameInformation,           // 48
    FileNetworkPhysicalNameInformation,      // 49 
    FileIdGlobalTxDirectoryInformation,      // 50
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#define INIT_UNICODE_STRING_WITH_BUF(_var, _buf, _buf_len) \
  _var.Buffer = _buf;                                      \
  StringCchLength(_buf, _buf_len, (size_t *)&_var.Length); \
  _var.Length *= sizeof(WCHAR);                            \
  _var.MaximumLength = _buf_len;

typedef struct _FILE_NAMES_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  PVOID RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef NTSTATUS (NTAPI *NT_OPEN_FILE_FUNC)(
  HANDLE *FileHandle,
  ACCESS_MASK DesiredAccess,
  POBJECT_ATTRIBUTES ObjectAttributes,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG ShareAccess,
  ULONG OpenOptions
);

typedef NTSTATUS (NTAPI *NT_QUERY_DIRECTORY_FILE_FUNC)(
  HANDLE FileHandle,
  HANDLE Event,
  PIO_APC_ROUTINE ApcRoutine,
  PVOID ApcContext,
  PIO_STATUS_BLOCK IoStatusBlock,
  PVOID FileInformation,
  ULONG Length,
  FILE_INFORMATION_CLASS FileInformationClass,
  BOOLEAN ReturnSingleEntry,
  PUNICODE_STRING FileName,
  BOOLEAN RestartScan
);

typedef struct _LOCAL_FILE_NAME_INFO {
  FILE_NAMES_INFORMATION Header;
  WCHAR Buffer[MAX_PATH];
} LOCAL_FILE_NAME_INFO;

VOID InitializeObjectAttributes(
  OUT POBJECT_ATTRIBUTES InitializedAttributes,
  IN PUNICODE_STRING ObjectName,
  IN ULONG Attributes,
  IN HANDLE RootDirectory,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

extern NT_OPEN_FILE_FUNC my_nt_open_file_func;

extern NT_QUERY_DIRECTORY_FILE_FUNC my_nt_query_directory_file_func;

int sqlpal_init_nt_open_file_and_nt_query_directory_file();

int sqlpal_nt_query_directory_file_is_available();

HANDLE sqlpal_opendir(const char *path);

int sqlpal_readdir_next_file(HANDLE dir, char *file_name, size_t file_name_len);

int sqlpal_closedir(HANDLE dir);

#ifdef __cplusplus
}
#endif

#endif /* SQLPAL_DIR_H */

#endif /* !DISABLE_LIST_DIR_NAME && _WIN32 */