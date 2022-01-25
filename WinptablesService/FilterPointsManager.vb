'
' File Name:		FilterPointsManager.vb
' Description:		Provide filter function
' Date:			    2022.1.23
' Author:			HBSnail
'

Imports System.Collections.Concurrent
Imports System.Net.NetworkInformation
Imports System.Reflection
Imports WinptablesService.RegistryUtils

Public Class FilterPointsManager

    Public Structure NetworkInterfaceProperty
        Public localIPAddress As List(Of Byte())
        Public macAddress As Byte()
    End Structure

    Public PreroutingFilterModulesChain As List(Of FilterModulesInfo)
    Public ForwardFilterModulesChain As List(Of FilterModulesInfo)
    Public InputFilterModulesChain As List(Of FilterModulesInfo)
    Public OutputFilterModulesChain As List(Of FilterModulesInfo)
    Public PostroutingFilterModulesChain As List(Of FilterModulesInfo)

    Public IfTables As ConcurrentDictionary(Of UInteger, NetworkInterfaceProperty)

    Public Function InitManager() As Boolean

        IfTables = New ConcurrentDictionary(Of UInteger, NetworkInterfaceProperty)
        FlashIfTable()

        AddHandler NetworkChange.NetworkAddressChanged, AddressOf IpaddressChange


        PreroutingFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.PREROUTING)
        ForwardFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.FORWARD)
        InputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.INPUT)
        OutputFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.OUTPUT)
        PostroutingFilterModulesChain = ReadWinptablesFilteringChain(FilterPoint.POSTROUTING)


        If PreroutingFilterModulesChain Is Nothing OrElse
        ForwardFilterModulesChain Is Nothing OrElse
        InputFilterModulesChain Is Nothing OrElse
        OutputFilterModulesChain Is Nothing OrElse
        PostroutingFilterModulesChain Is Nothing Then

            Return False

        End If

        Return True
    End Function

    Public Function IsLocalIfIPAddr(ifIndex As UInteger, dstIPaddr As Byte()) As Boolean
        If dstIPaddr(0) = 255 AndAlso
                dstIPaddr(1) = 255 AndAlso
                dstIPaddr(2) = 255 AndAlso
                dstIPaddr(3) = 255 Then
            Return True
        End If
        If IfTables.ContainsKey(ifIndex) Then
            For Each ip As Byte() In IfTables(ifIndex).localIPAddress
                If BytesOperator.BytesEqual(dstIPaddr, ip) Then
                    Return True
                End If
            Next
        Else
            FlashIfTable()
            Return True
        End If
        Return False
    End Function

    Public Sub IpaddressChange(sender As Object, e As EventArgs)
        FlashIfTable()
    End Sub

    Public Sub FlashIfTable()
        IfTables.Clear()

        For Each NICInfo As NetworkInterface In NetworkInterface.GetAllNetworkInterfaces

            Dim currentNICInfo As NetworkInterfaceProperty


            Dim currentIfIndex As UInteger = CUInt(NICInfo.GetType.GetField("index", BindingFlags.NonPublic Or BindingFlags.Instance).GetValue(NICInfo))

            Dim ipList As New List(Of Byte())
            For Each i As UnicastIPAddressInformation In NICInfo.GetIPProperties.UnicastAddresses
                ipList.Add(i.Address.GetAddressBytes())
            Next
            For Each i As MulticastIPAddressInformation In NICInfo.GetIPProperties.MulticastAddresses
                ipList.Add(i.Address.GetAddressBytes())
            Next
            For Each i As IPAddressInformation In NICInfo.GetIPProperties.AnycastAddresses
                ipList.Add(i.Address.GetAddressBytes())
            Next


            currentNICInfo.localIPAddress = ipList
            currentNICInfo.macAddress = NICInfo.GetPhysicalAddress.GetAddressBytes

            IfTables.TryAdd(currentIfIndex, currentNICInfo)

        Next
    End Sub

    Public Function ProcessPrerouting(packets As Byte()) As Byte()
        Try
            If packets IsNot Nothing Then
                For Each fp As FilterModulesInfo In PreroutingFilterModulesChain
                    packets = fp.processLib.GetType.GetMethod("WinptablesPreroutingFilterPoint").Invoke(fp.processLib, {packets})
                Next
            End If
        Catch
        End Try
        Return packets
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


    Public Function ProcessPostrouting(packets As Byte()) As Byte()
        Try
            If packets IsNot Nothing Then
                For Each fp As FilterModulesInfo In PostroutingFilterModulesChain
                    packets = fp.processLib.GetType.GetMethod("WinptablesPostroutingFilterPoint").Invoke(fp.processLib, {packets})
                Next
            End If
        Catch
        End Try
        Return packets
    End Function

    Public Function ProcessForward(packets As Byte()) As Byte()
        Try
            If packets IsNot Nothing Then
                For Each fp As FilterModulesInfo In ForwardFilterModulesChain
                    packets = fp.processLib.GetType.GetMethod("WinptablesForwardFilterPoint").Invoke(fp.processLib, {packets})
                Next
            End If
        Catch
        End Try
        Return packets
    End Function

End Class

