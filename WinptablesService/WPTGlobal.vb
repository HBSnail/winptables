Public Module WPTGlobal

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

    Public Enum EthernetTypeCode As UInteger
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
