'
' File Name:		WinptablesService.vb
' Description:		Entry point for WinptablesService
' Date:			    2022.1.15
' Author:			HBSnail
'

Imports System.IO
Imports System.Net
Imports System.Runtime.InteropServices
Imports Microsoft.Win32.SafeHandles
Imports WinptablesService.RegistryUtils

Public Module Main


    Public winptablesDeviceHandle As SafeFileHandle
    Public deviceStream As FileStream
    Public buffer(65536) As Byte

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"

    Public filterpointMgr As New FilterPointsManager()
    Public routingMgr As New RoutingManager()
    Public pack As Long = 0


    Sub Main(ByVal args() As String)



        winptablesDeviceHandle = New SafeFileHandle(CreateFile(WINPTABLES_DEVICE_NAME,
                                                               FileAccess.ReadWrite,
                                                               FileShare.None,
                                                               IntPtr.Zero,
                                                               FileMode.Open,
                                                               FileAttributesAndFlags.Attribute_System Or FileAttributesAndFlags.Flag_Overlapped,
                                                               IntPtr.Zero),
                                                    True)

        If winptablesDeviceHandle.IsInvalid Then
            Console.WriteLine("ERR")
            Exit Sub
        End If

        deviceStream = New FileStream(winptablesDeviceHandle, FileAccess.ReadWrite, 65536, True)
        deviceStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), deviceStream)

        Console.Read()
    End Sub

    Private Sub readFromWinptablesDevice(ar As IAsyncResult)
        Dim dStream As FileStream = ar.AsyncState
        Dim readLength As Integer = dStream.EndRead(ar)


        If readLength > 9 Then '1Byte 2Uint and data 

            If (buffer(0) = TRANSFER_DIRECION.NICToFilter) Then

                buffer(0) = TRANSFER_DIRECION.FilterToUpper
            ElseIf (buffer(0) = TRANSFER_DIRECION.UpperToFilter) Then

                buffer(0) = TRANSFER_DIRECION.FilterToNIC
            Else
                'Not a valid TRANSFER_DIRECION.
                'Just drop this packet - skip transmit to winptables kernel driver process.
                Return
            End If

            deviceStream.Write(buffer, 0, buffer.Length)

            'Before receive we must finish copied the data to processBuffer or we will lost the data
            dStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), dStream)


        Else
            dStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), dStream)
        End If


    End Sub

    Private Sub ProcessEthFrame(ethFrameBuffer As Byte(), direction As TRANSFER_DIRECION, ifIndex As UInteger)
        Try
            If (direction = TRANSFER_DIRECION.NICToFilter) Then

                SendEthFarmes2Kernel(ethFrameBuffer, TRANSFER_DIRECION.FilterToUpper, ifIndex)
            ElseIf (direction = TRANSFER_DIRECION.UpperToFilter) Then

                SendEthFarmes2Kernel(ethFrameBuffer, TRANSFER_DIRECION.FilterToNIC, ifIndex)
            Else
                'Not a valid TRANSFER_DIRECION.
                'Just drop this packet - skip transmit to winptables kernel driver process.
                Return
            End If
        Catch
        End Try
    End Sub

    Public Sub SendEthFarmes2Kernel(ethFrames As Byte(), direction As TRANSFER_DIRECION, ifIndex As UInteger)

        'One of the filter module return NULL exit (Drop)
        If ethFrames Is Nothing Then
            Return
        End If


        If direction = TRANSFER_DIRECION.NICToFilter OrElse direction = TRANSFER_DIRECION.UpperToFilter Then
            Return
        End If

        Dim sendBuffer(ethFrames.Length - 1 + 9) As Byte
        sendBuffer(0) = direction
        sendBuffer(1) = ifIndex And &HFF
        sendBuffer(1 + 1) = (ifIndex >> 8) And &HFF
        sendBuffer(1 + 2) = (ifIndex >> 16) And &HFF
        sendBuffer(1 + 3) = (ifIndex >> 24) And &HFF

        sendBuffer(5) = ethFrames.Length And &HFF
        sendBuffer(5 + 1) = (ethFrames.Length >> 8) And &HFF
        sendBuffer(5 + 2) = (ethFrames.Length >> 16) And &HFF
        sendBuffer(5 + 3) = (ethFrames.Length >> 24) And &HFF

        Array.Copy(ethFrames, 0, sendBuffer, 9, ethFrames.Length)

        deviceStream.Write(sendBuffer, 0, sendBuffer.Length)

    End Sub


End Module
