Imports System.Collections.Concurrent
Imports System.Net.NetworkInformation
Imports System.Reflection

Public Class RoutingManager

    Public IfTables As ConcurrentDictionary(Of UInteger, Byte())

    Public Sub New()

        IfTables = New ConcurrentDictionary(Of UInteger, Byte())
        FlashIfTable()

        AddHandler NetworkChange.NetworkAddressChanged, AddressOf IpaddressChange

    End Sub


    Public Sub IpaddressChange(sender As Object, e As EventArgs)
        FlashIfTable()
    End Sub

    Public Sub FlashIfTable()
        IfTables.Clear()

        For Each NICInfo As NetworkInterface In NetworkInterface.GetAllNetworkInterfaces


            Dim currentIfIndex As UInteger = CUInt(NICInfo.GetType.GetField("index", BindingFlags.NonPublic Or BindingFlags.Instance).GetValue(NICInfo))

            Dim ipList As New LinkedList(Of Byte())
            For Each i As UnicastIPAddressInformation In NICInfo.GetIPProperties.UnicastAddresses
                ipList.AddLast(i.Address.GetAddressBytes())
            Next

            IfTables.TryAdd(currentIfIndex, NICInfo.GetPhysicalAddress.GetAddressBytes)

        Next
    End Sub

    Public Function GetInterfaceMACByIfIndex(IfIndex As UInteger) As Byte()
        Try
            If IfTables.ContainsKey(IfIndex) Then
                Return IfTables(IfIndex)
            End If
        Catch
        End Try
        Return Nothing
    End Function
End Class
