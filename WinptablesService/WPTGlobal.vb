Imports System.Collections.Concurrent

Public Module WPTGlobal


    Public Structure ForwardTableItem
        Public InterfaceIndex As UInteger
        Public EthernetDstSrcAddress() As Byte
        Public Shared Operator =(a As ForwardTableItem, b As ForwardTableItem) As Boolean
            Return (BytesOperator.BytesEqual(a.EthernetDstSrcAddress, b.EthernetDstSrcAddress) AndAlso (a.InterfaceIndex = b.InterfaceIndex))
        End Operator
        Public Shared Operator <>(a As ForwardTableItem, b As ForwardTableItem) As Boolean
            Return Not (BytesOperator.BytesEqual(a.EthernetDstSrcAddress, b.EthernetDstSrcAddress) AndAlso (a.InterfaceIndex = b.InterfaceIndex))
        End Operator
    End Structure

    Public ModuleList As New Dictionary(Of String, Object)

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"

    Public fpMgr As New FilterPointsManager()

    Public globalForwardEnable As Boolean = True

    Public Enum TRANSFER_DIRECION
        NICToFilter
        FilterToUpper
        UpperToFilter
        FilterToNIC
    End Enum

    Public Enum DST_ADDR_TYPE As Byte
        LOCAL
        NOT_LOCAL
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


End Module
