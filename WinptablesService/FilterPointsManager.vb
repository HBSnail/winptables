'
' File Name:		FilterPointsManager.vb
' Description:		Provide filter function
' Date:			    2022.1.23
' Author:			HBSnail
'

Imports WinptablesService.RegistryUtils

Public Class FilterPointsManager

    Public InputFilterModulesChain As List(Of FilterModulesInfo)
    Public OutputFilterModulesChain As List(Of FilterModulesInfo)


    Public Function InitManager() As Boolean

        InputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.INPUT)
        OutputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.OUTPUT)

        If InputFilterModulesChain Is Nothing OrElse
        OutputFilterModulesChain Is Nothing Then
            Return False
        End If

        Return True
    End Function


    Public Function ProcessInput(packets As Byte()) As Byte()
        Try
            If packets IsNot Nothing Then
                For Each fp As FilterModulesInfo In InputFilterModulesChain
                    packets = fp.processLib.GetType.GetMethod("WinptablesInputFilterPoint").Invoke(fp.processLib, {packets})
                Next
            End If
        Catch
        End Try
        Return packets
    End Function

    Public Function ProcessOutput(packets As Byte()) As Byte()
        Try
            If packets IsNot Nothing Then
                For Each fp As FilterModulesInfo In OutputFilterModulesChain
                    packets = fp.processLib.GetType.GetMethod("WinptablesOutputFilterPoint").Invoke(fp.processLib, {packets})
                Next
            End If
        Catch
        End Try
        Return packets
    End Function


End Class

