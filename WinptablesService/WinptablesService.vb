'
' File Name:		WinptablesService.vb
' Description:		Entry point for WinptablesService
' Date:			    2022.1.15
' Author:			HBSnail
'

Imports System.IO
Imports Microsoft.Win32.SafeHandles
Imports WinptablesService.RegistryUtils

Public Class WinptablesService


    Public winptablesDeviceHandle As SafeFileHandle
    Public deviceStream As FileStream
    Public buffer(65536) As Byte

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"

    Public PreroutingFilterModulesChain As List(Of FilterModulesInfo)
    Public ForwardFilterModulesChain As List(Of FilterModulesInfo)
    Public InputFilterModulesChain As List(Of FilterModulesInfo)
    Public OutputFilterModulesChain As List(Of FilterModulesInfo)
    Public PostroutingFilterModulesChain As List(Of FilterModulesInfo)

    Protected Overrides Sub OnStart(ByVal args() As String)

        'Create necessary registry
        If Not CreateWinptablesRegistryItems() Then
            Me.Stop()
            Exit Sub
        End If

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

        'Get saved data from windows registry
        PreroutingFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.PREROUTING)
        ForwardFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.FORWARD)
        InputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.INPUT)
        OutputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.OUTPUT)
        PostroutingFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.POSTROUTING)

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
