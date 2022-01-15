'
' File Name:		RegistryUtils.vb
' Description:		Provide necessary operation for windows registry to WinptablesService
' Date:			    2022.1.15
' Author:			HBSnail
'

Imports Microsoft.Win32

Public Class RegistryUtils

    Public Enum FilterPoint As Byte
        PREROUTING
        FORWARD
        INPUT
        OUTPUT
        POSTROUTING
    End Enum

    Public Structure FilterModulesInfo
        Public priority As UInteger
        Public modulePath As String

        Public Sub New(p As UInteger, m As Object)
            priority = p
            modulePath = m
        End Sub
    End Structure


    'NOTICE: The keys will store in "Computer\HKEY_LOCAL_MACHINE\SOFTWARE\icSecLab\winptables"
    'SubKeys -  PREROUTING,FORWARD,INPUT,OUTPUT,POSTROUTING
    '           5 filtering points module lists config
    Public Shared Function CreateWinptablesRegistryItems() As Boolean

        Try
            Dim winptablesKey As RegistryKey = Registry.LocalMachine.CreateSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)
            winptablesKey.CreateSubKey("PREROUTING")
            winptablesKey.CreateSubKey("FORWARD")
            winptablesKey.CreateSubKey("INPUT")
            winptablesKey.CreateSubKey("OUTPUT")
            winptablesKey.CreateSubKey("POSTROUTING")
        Catch
            Return False
        End Try

        Return True

    End Function

    'NOTICE:    Larger priority in number is in lower priority.
    '           It indecates the order for the filter link list.
    Public Shared Function WriteWinptablesRegistryItems(filterPoint As FilterPoint, priority As UInteger, modulePath As String) As Boolean

        Try

            Dim winptablesKey As RegistryKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)

            Select Case filterPoint
                Case FilterPoint.PREROUTING
                    winptablesKey.OpenSubKey("PREROUTING").SetValue(priority.ToString, modulePath, RegistryValueKind.String)
                Case FilterPoint.INPUT
                    winptablesKey.OpenSubKey("INPUT").SetValue(priority.ToString, modulePath, RegistryValueKind.String)
                Case FilterPoint.FORWARD
                    winptablesKey.OpenSubKey("FORWARD").SetValue(priority.ToString, modulePath, RegistryValueKind.String)
                Case FilterPoint.OUTPUT
                    winptablesKey.OpenSubKey("OUTPUT").SetValue(priority.ToString, modulePath, RegistryValueKind.String)
                Case FilterPoint.POSTROUTING
                    winptablesKey.OpenSubKey("POSTROUTING").SetValue(priority.ToString, modulePath, RegistryValueKind.String)
                Case Else
                    Return False
            End Select

        Catch
            Return False
        End Try

        Return True

    End Function

    Public Shared Function DeleteWinptablesRegistryItems(filterPoint As FilterPoint, priority As UInteger) As Boolean

        Try

            Dim winptablesKey As RegistryKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)

            Select Case filterPoint
                Case FilterPoint.PREROUTING
                    winptablesKey.OpenSubKey("PREROUTING").DeleteValue(priority.ToString, False)
                Case FilterPoint.INPUT
                    winptablesKey.OpenSubKey("INPUT").DeleteValue(priority.ToString, False)
                Case FilterPoint.FORWARD
                    winptablesKey.OpenSubKey("FORWARD").DeleteValue(priority.ToString, False)
                Case FilterPoint.OUTPUT
                    winptablesKey.OpenSubKey("OUTPUT").DeleteValue(priority.ToString, False)
                Case FilterPoint.POSTROUTING
                    winptablesKey.OpenSubKey("POSTROUTING").DeleteValue(priority.ToString, False)
                Case Else
                    Return False
            End Select

        Catch
            Return False
        End Try

        Return True

    End Function

    Public Shared Function ReadWinptablesFilteringChain(filterPoint As FilterPoint) As List(Of FilterModulesInfo)
        Dim filterChain As List(Of FilterModulesInfo)

        Try

            Dim opKey As RegistryKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)

            Select Case filterPoint
                Case FilterPoint.PREROUTING
                    opKey = opKey.OpenSubKey("PREROUTING")
                Case FilterPoint.INPUT
                    opKey = opKey.OpenSubKey("INPUT")
                Case FilterPoint.FORWARD
                    opKey = opKey.OpenSubKey("FORWARD")
                Case FilterPoint.OUTPUT
                    opKey = opKey.OpenSubKey("OUTPUT")
                Case FilterPoint.POSTROUTING
                    opKey = opKey.OpenSubKey("POSTROUTING")
                Case Else
                    Return Nothing
            End Select

            filterChain = New List(Of FilterModulesInfo)

            For Each i As String In opKey.GetValueNames()
                filterChain.Add(New FilterModulesInfo(CUInt(i), opKey.GetValue(i)))
            Next

            filterChain.Sort()

        Catch
            Return Nothing
        End Try

        Return filterChain
    End Function

    Public Shared Function WriteWinptablesFilteringChain(filterPoint As FilterPoint, chain As List(Of FilterModulesInfo)) As Boolean
        Try

            Dim opKey As RegistryKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)

            Select Case filterPoint
                Case FilterPoint.PREROUTING
                    opKey = opKey.OpenSubKey("PREROUTING")
                Case FilterPoint.INPUT
                    opKey = opKey.OpenSubKey("INPUT")
                Case FilterPoint.FORWARD
                    opKey = opKey.OpenSubKey("FORWARD")
                Case FilterPoint.OUTPUT
                    opKey = opKey.OpenSubKey("OUTPUT")
                Case FilterPoint.POSTROUTING
                    opKey = opKey.OpenSubKey("POSTROUTING")
                Case Else
                    Return False
            End Select


            For Each i As FilterModulesInfo In chain
                opKey.SetValue(i.priority, i.modulePath)
            Next

        Catch
            Return False
        End Try

        Return True

    End Function

End Class

