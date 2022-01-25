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

    Protected Overrides Sub OnStart(ByVal args() As String)

        'Create necessary registry
        If Not CreateWinptablesRegistryItems() Then
            Me.Stop()
            Exit Sub
        End If

        'Get saved data from windows registry
        If Not fpMgr.InitManager() Then
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

        deviceStream = New FileStream(winptablesDeviceHandle, FileAccess.ReadWrite, 65536, True)
        deviceStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), deviceStream)


    End Sub

    Private Sub readFromWinptablesDevice(ar As IAsyncResult)
        Dim dStream As FileStream = ar.AsyncState
        Dim readLength As Integer = dStream.EndRead(ar)


        If readLength > 9 Then '1Byte 2Uint and data 

            Do

                Dim direction As Byte = buffer(0)
                Dim ifIndex As UInteger = BitConverter.ToUInt32(buffer, 1)
                Dim ethframeLength As UInteger = BitConverter.ToUInt32(buffer, 5)
                Dim ethFrameBuffer(ethframeLength - 1) As Byte
                Array.Copy(buffer, 9, ethFrameBuffer, 0, ethframeLength)

                'Before receive we must finish copied the data to processBuffer or we will lost the data
                dStream.BeginRead(buffer, 0, buffer.Length, New AsyncCallback(AddressOf readFromWinptablesDevice), dStream)


                If (direction = TRANSFER_DIRECION.NICToFilter) Then

                    'PREROUTING
                    ethFrameBuffer = fpMgr.ProcessPrerouting(ethFrameBuffer)


                    'If the direction is match to the host process the INPUT,otherwise
                    'check IF THE FORWARD ENABLE and call FORWARD AND POSTROUTING
                    '
                    '
                    'The code may be like this:
                    '
                    'If IP.DEST = LOCAL Then
                    '   PACKETS() = ProcessINPUT(Packet)
                    '   TRANSMIT2UPPER(PACKETS())
                    'Else
                    '   If FORWARD_ENABLE Then
                    '       PACKETS() = ProcessFORWARD(Packet)
                    '       PACKETS() = ProcessPOSTROUTING(ALL PACKET IN PACKETS())
                    '       TRANSMIT2NIC(PACKETS())
                    '   Else
                    '       'Just drop this packet - skip transmit to winptables kernel driver process.
                    '       Exit Do
                    '   End If
                    'End IF

                    'Resolve the ethFrame is in DIX Ethernet V2 Frame (Ethernet II)
                    'In experiment we never see any IEEE802.3 Ethernet frame on our windows machine
                    'DIXv2 Ethernet
                    'SrcMAC-6byte|DstMAC-6Byte|TypeCode-2Byte|Payload|FCS-4Byte
                    'Before the frame was captured by our filter driver, the NIC will chech FCS and we will not get it(DO NOT CARE)
                    '
                    'NOTICE: All data in network is transfered in BIG ENDIAN so we need convert first.

                    Dim currentEthFrameTypeCode As EthernetTypeCode = BytesOperator.GetBigEndianUShort(ethFrameBuffer, 12)
                    Dim dstAddrNotLocal As Boolean = False

                    'We can only forward the IPv4&IPv6 packets
                    If currentEthFrameTypeCode = EthernetTypeCode.IP Then

                        'Resolve as IPv4 Packet (RFC791)
                        If Not fpMgr.IsLocalIfIPAddr(ifIndex, {ethFrameBuffer(30), ethFrameBuffer(31), ethFrameBuffer(32), ethFrameBuffer(33)}) Then
                            dstAddrNotLocal = True
                        End If

                    ElseIf currentEthFrameTypeCode = EthernetTypeCode.IPv6 Then

                        'Resolve as IPv6 Packet (RFC2460)
                        If Not fpMgr.IsLocalIfIPAddr(ifIndex, {ethFrameBuffer(38), ethFrameBuffer(39), ethFrameBuffer(40), ethFrameBuffer(41),
                                                         ethFrameBuffer(42), ethFrameBuffer(43), ethFrameBuffer(44), ethFrameBuffer(45),
                                                         ethFrameBuffer(46), ethFrameBuffer(47), ethFrameBuffer(48), ethFrameBuffer(49),
                                                         ethFrameBuffer(50), ethFrameBuffer(51), ethFrameBuffer(52), ethFrameBuffer(53)}) Then
                            dstAddrNotLocal = True
                        End If

                    End If


                    If dstAddrNotLocal And globalForwardEnable Then

                        'FORWARD
                        ethFrameBuffer = fpMgr.ProcessForward(ethFrameBuffer)

                        'POSTROUTING
                        ethFrameBuffer = fpMgr.ProcessPostrouting(ethFrameBuffer)

                        SendEthFarmes2Kernel(ethFrameBuffer, TRANSFER_DIRECION.FilterToNIC, ifIndex, dStream)

                    Else

                        'INPUT
                        ethFrameBuffer = fpMgr.ProcessInput(ethFrameBuffer)
                        SendEthFarmes2Kernel(ethFrameBuffer, TRANSFER_DIRECION.FilterToUpper, ifIndex, dStream)

                    End If


                ElseIf (direction = TRANSFER_DIRECION.UpperToFilter) Then

                    'First call OUTPUT and then POSTROUTING
                    'PACKETS() = ProcessOUTPUT(Packet)
                    'PACKETS() = ProcessPOSTROUTING(ALL PACKET IN PACKETS())
                    'TRANSMIT2NIC(PACKETS)

                    'OUTPUT
                    ethFrameBuffer = fpMgr.ProcessOutput(ethFrameBuffer)

                    'POSTROUTING
                    ethFrameBuffer = fpMgr.ProcessPostrouting(ethFrameBuffer)

                    SendEthFarmes2Kernel(ethFrameBuffer, TRANSFER_DIRECION.FilterToNIC, ifIndex, dStream)

                Else
                    'Not a valid TRANSFER_DIRECION.
                    'Just drop this packet - skip transmit to winptables kernel driver process.
                    Exit Do
                End If




            Loop While False

        End If


    End Sub
    Private Sub SendEthFarmes2Kernel(ethFrames As Byte(), direction As TRANSFER_DIRECION, ifIndex As UInteger, dStream As FileStream)

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

        dStream.Write(sendBuffer, 0, sendBuffer.Length)

    End Sub

    Protected Overrides Sub OnStop()
        deviceStream.Close()
    End Sub

End Class
