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
        INPUT
        OUTPUT
        POSTROUTING
    End Enum

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
                    Dim mount As Boolean = targetT.GetMethod("WinptablesModuleCreate").Invoke(targetObj, Nothing)
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

    Private Shared Function FilterModulesCompare(x As FilterModulesInfo, y As FilterModulesInfo) As Integer
        Return (x.priority - y.priority)
    End Function

    'NOTICE: The keys will store in "Computer\HKEY_LOCAL_MACHINE\SOFTWARE\icSecLab\winptables"
    'SubKeys -  PREROUTING,INPUT,OUTPUT,POSTROUTING
    '           5 filtering points module lists config
    Public Shared Function CreateWinptablesRegistryItems() As Boolean

        Try
            Dim winptablesKey As RegistryKey = Registry.LocalMachine.CreateSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)
            winptablesKey.CreateSubKey("PREROUTING")
            winptablesKey.CreateSubKey("INPUT")
            winptablesKey.CreateSubKey("OUTPUT")
            winptablesKey.CreateSubKey("POSTROUTING")
        Catch
            Return False
        End Try

        Return True

    End Function

    Public Shared Function OpenOpRegistryKey(filterPoint As FilterPoint)
        Dim opKey As RegistryKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)

        Select Case filterPoint
            Case FilterPoint.PREROUTING
                opKey = opKey.OpenSubKey("PREROUTING", RegistryKeyPermissionCheck.ReadWriteSubTree)
            Case FilterPoint.INPUT
                opKey = opKey.OpenSubKey("INPUT", RegistryKeyPermissionCheck.ReadWriteSubTree)
            Case FilterPoint.OUTPUT
                opKey = opKey.OpenSubKey("OUTPUT", RegistryKeyPermissionCheck.ReadWriteSubTree)
            Case FilterPoint.POSTROUTING
                opKey = opKey.OpenSubKey("POSTROUTING", RegistryKeyPermissionCheck.ReadWriteSubTree)
            Case Else
                Return Nothing
        End Select


        If opKey Is Nothing Then
            'Create missing keys
            CreateWinptablesRegistryItems()

            'Try again
            opKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\icSecLab\winptables", RegistryKeyPermissionCheck.ReadWriteSubTree)
            Select Case filterPoint
                Case FilterPoint.PREROUTING
                    opKey = opKey.OpenSubKey("PREROUTING", RegistryKeyPermissionCheck.ReadWriteSubTree)
                Case FilterPoint.INPUT
                    opKey = opKey.OpenSubKey("INPUT", RegistryKeyPermissionCheck.ReadWriteSubTree)
                Case FilterPoint.OUTPUT
                    opKey = opKey.OpenSubKey("OUTPUT", RegistryKeyPermissionCheck.ReadWriteSubTree)
                Case FilterPoint.POSTROUTING
                    opKey = opKey.OpenSubKey("POSTROUTING", RegistryKeyPermissionCheck.ReadWriteSubTree)
                Case Else
                    Return Nothing
            End Select

        End If

        Return opKey
    End Function

    'NOTICE:    Larger priority in number is in lower priority.
    '           It indecates the order for the filter link list.
    Public Shared Function WriteWinptablesRegistryItems(filterPoint As FilterPoint, priority As UInteger, modulePath As String) As Boolean

        Try
            OpenOpRegistryKey(filterPoint).SetValue(priority.ToString, modulePath, RegistryValueKind.String)
        Catch
            Return False
        End Try

        Return True

    End Function

    Public Shared Function DeleteWinptablesRegistryItems(filterPoint As FilterPoint, priority As UInteger) As Boolean

        Try
            OpenOpRegistryKey(filterPoint).DeleteValue(priority.ToString, False)
        Catch
            Return False
        End Try

        Return True

    End Function

    Public Shared Function ReadWinptablesFilteringChain(filterPoint As FilterPoint) As List(Of FilterModulesInfo)
        Dim filterChain As List(Of FilterModulesInfo)

        Try

            Dim opKey As RegistryKey = OpenOpRegistryKey(filterPoint)

            filterChain = New List(Of FilterModulesInfo)

            For Each i As String In opKey.GetValueNames()
                Dim filterPointInfo As FilterModulesInfo = New FilterModulesInfo(CUInt(i), opKey.GetValue(i))
                If filterPointInfo.processLib IsNot Nothing Then
                    filterChain.Add(filterPointInfo)
                End If

            Next

            filterChain.Sort(New Comparison(Of FilterModulesInfo)(AddressOf FilterModulesCompare))

        Catch
            Return Nothing
        End Try

        Return filterChain
    End Function

    Public Shared Function WriteWinptablesFilteringChain(filterPoint As FilterPoint, chain As List(Of FilterModulesInfo)) As Boolean

        Try

            Dim opKey As RegistryKey = OpenOpRegistryKey(filterPoint)

            For Each i As FilterModulesInfo In chain
                opKey.SetValue(i.priority.ToString, i.modulePath.ToString, RegistryValueKind.String)
            Next

        Catch
            Return False
        End Try

        Return True

    End Function

End Class

