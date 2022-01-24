Module WPTGlobal

    Public ModuleList As New Dictionary(Of String, Object)

    Public Const WINPTABLES_DEVICE_NAME As String = "\\.\winptables_comm"

    Public fpMgr As New FilterPointsManager()

    Public Enum TRANSFER_DIRECION
        NICToFilter
        FilterToUpper
        UpperToFilter
        FilterToNIC
    End Enum

End Module
