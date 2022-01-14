Imports System.IO
Imports System.IO.Pipes
Imports Microsoft.Win32.SafeHandles

Public Class WinptablesService


    Public winptablesDeviceHandle As SafeFileHandle
    Public deviceStream As FileStream
    Public buffer(65536) As Byte

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"


    Protected Overrides Sub OnStart(ByVal args() As String)
        ' Add code here to start your service. This method should set things
        ' in motion so your service can do its work.

        winptablesDeviceHandle = New SafeFileHandle(CreateFile(WINPTABLES_DEVICE_NAME,
                                                               FileAccess.ReadWrite,
                                                               FileShare.None,
                                                               IntPtr.Zero,
                                                               FileMode.Open,
                                                               FileAttributesAndFlags.Attribute_System Or FileAttributesAndFlags.Flag_Overlapped,
                                                               IntPtr.Zero),
                                                    True)

        If winptablesDeviceHandle.IsInvalid Then
            Me.Stop()
            Exit Sub
        End If


        deviceStream = New FileStream(winptablesDeviceHandle, FileAccess.ReadWrite, 65536, True)
        deviceStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), deviceStream)


    End Sub

    Private Sub readFromWinptablesDevice(ar As IAsyncResult)
        Dim dStream As FileStream = ar.AsyncState
        Dim readLength As Integer = dStream.EndRead(ar)
        If readLength > 0 Then

            If (buffer(0) = 0) Then
                buffer(0) = 1
            Else
                buffer(0) = 3
            End If

            dStream.Write(buffer, 0, buffer.Length)
        End If

        dStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), dStream)

    End Sub

    Protected Overrides Sub OnStop()
        deviceStream.Close()
    End Sub

End Class
