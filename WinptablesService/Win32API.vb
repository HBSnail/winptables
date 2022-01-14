Imports System.IO
Imports System.Runtime.InteropServices

Module Win32API
    Public Enum FileAttributesAndFlags
        Attribute_Archive = &H20
        Attribute_Encrypted = &H4000
        Attribute_Hidden = &H2
        Attribute_Normal = &H80
        Attribute_Offline = &H1000
        Attribute_ReadOnly = &H1
        Attribute_System = &H4
        Attribute_Temporary = &H100

        Flag_BackupSemantics = &H2000000
        Flag_DeleteOnClose = &H4000000
        Flag_NoBuffering = &H20000000
        Flag_OpenNoRecall = &H100000
        Flag_OpenReparsePoint = &H200000
        Flag_Overlapped = &H40000000
        Flag_PosixSemantics = &H1000000
        Flag_RandomAccess = &H10000000
        Flag_SessionAware = &H800000
        Flag_SequentialScan = &H8000000
        Flag_WriteThrough = &H80000000
    End Enum



    Public Declare Unicode Function CreateFile Lib "kernel32" Alias "CreateFileW" (lpFileName As String,
                                                                                   <MarshalAs(UnmanagedType.U4)> dwDesiredAccess As FileAccess,
                                                                                   <MarshalAs(UnmanagedType.U4)> dwShareMode As FileShare,
                                                                                   lpSecurityAttributes As IntPtr,
                                                                                   <MarshalAs(UnmanagedType.U4)> dwCreationDisposition As FileMode,
                                                                                   <MarshalAs(UnmanagedType.U4)> dwFlagsAndAttributes As FileAttributesAndFlags,
                                                                                   hTemplateFile As IntPtr) As IntPtr


    Public Declare Unicode Function DeviceIoControl Lib "kernel32" Alias "DeviceIoControl" (hDevice As IntPtr,
                                                                                            dwIoControlCode As UInteger,
                                                                                            lpInBuffer As IntPtr,
                                                                                            nInBufferSize As UInteger,
                                                                                            lpOutBuffer As IntPtr,
                                                                                            nOutBufferSize As UInteger,
                                                                                            ByRef lpBytesReturned As UInteger,
                                                                                            lpOverlapped As IntPtr) As Boolean


End Module

