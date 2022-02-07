Imports System.Collections.Concurrent

Public Module WPTGlobal


    Public ModuleList As New Dictionary(Of String, Object)

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"


    Public globalForwardEnable As Boolean = True

    Public Enum TRANSFER_DIRECION
        NICToFilter
        FilterToUpper
        UpperToFilter
        FilterToNIC
    End Enum


    Public Enum ETHERNET_TYPE_CODE As UInteger
        IP = &H800
        ARP = &H806
        FrameRelayARP = &H808
        DRARP = &H8035
        RARP = &H8035
        AARP = &H80F3
        EAPS = &H8100
        IPX = &H8137
        SNMP = &H814C
        IPv6 = &H86DD
        MPCP = &H8808
        PPP = &H880B
        PPPoE_Discovery = &H8863
        PPPoE_Session = &H8864
        EAPOL = &H888E
        LLDP = &H88CC
        VLAN = &H8100
    End Enum

    Public Enum FilterPoint As Byte
        PREROUTING
        FORWARD
        INPUT
        OUTPUT
        POSTROUTING
    End Enum

    Public Enum AddressFamily As Short
        InterNetwork = 2
        InterNetworkV6 = 23
    End Enum

    Public Structure NetworkInterfaceProperty
        Public localIPAddress As List(Of Byte())
        Public macAddress As Byte()
    End Structure

    Public Structure FilterModulesInfo
        'Priority type here is set to LONG(64bit) mismatch the priority type UINT(32bit) from functions
        'because we do not want to convert the type when comparing 2 FilterModulesInfo,
        'otherwise we need convert type there to avoid overflow.
        Public priority As Long
        Public modulePath As String
        Public processLib As Object

        Public Sub New(p As UInteger, m As String)

            priority = p
            modulePath = m
            processLib = Nothing

            Try

                If ModuleList.ContainsKey(m) Then
                    processLib = ModuleList(m)
                    Return
                End If

                Dim targetT As Type = Nothing
                Dim asm As Reflection.Assembly = Reflection.Assembly.LoadFile(m)
                For Each T As Type In asm.GetTypes()
                    If T.Name = "WinptablesFilterModule" Then
                        targetT = T
                        Exit For
                    End If
                Next

                If targetT IsNot Nothing Then
                    Dim targetObj As Object = asm.CreateInstance(targetT.FullName)
                    Dim mount As Boolean = False
                    If mount Then
                        processLib = targetObj
                        ModuleList.Add(m, targetObj)
                    Else
                        targetT.GetMethod("WinptablesModuleDestory").Invoke(targetObj, Nothing)
                    End If
                End If
            Catch
            End Try
        End Sub
    End Structure

    Public Function FilterModulesCompare(x As FilterModulesInfo, y As FilterModulesInfo) As Integer
        Return (x.priority - y.priority)
    End Function

End Module
