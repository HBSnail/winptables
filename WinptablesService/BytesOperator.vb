Public Class BytesOperator

    Public Shared Function GetBigEndianUShort(ByRef idata As Byte(), ByVal offset As UInteger) As UShort
        If idata.Length - 1 < offset Then
            Throw New Exception("Offset out of boundary. DATA_LENGTH=" & idata.Length.ToString & " OFFSET=" & offset.ToString)
        End If
        Return (CUInt(idata(offset)) << 8) + idata(offset + 1)
    End Function
    Public Shared Sub WriteBigEndianUShort(ByRef idata As Byte(), ByVal offset As UInteger, insert As UShort)
        If idata.Length - 1 < offset Then
            Throw New Exception("Offset out of boundary. DATA_LENGTH=" & idata.Length.ToString & " OFFSET=" & offset.ToString)
        End If
        idata(offset) = (insert >> 8) And &HFF
        idata(offset + 1) = insert And &HFF
    End Sub


    Public Shared Function GetBigEndianUInt(ByRef idata As Byte(), ByVal offset As UInteger) As UInteger
        If idata.Length - 3 < offset Then
            Throw New Exception("Offset out of boundary. DATA_LENGTH=" & idata.Length.ToString & " OFFSET=" & offset.ToString)
        End If
        Return (CUInt(idata(offset)) << 24) + (CUInt(idata(offset + 1)) << 16) + (CUInt(idata(offset + 2)) << 8) + idata(offset + 3)
    End Function

    Public Shared Sub WriteBigEndianUInt(ByRef idata As Byte(), ByVal offset As UInteger, insert As UInteger)
        If idata.Length - 1 < offset Then
            Throw New Exception("Offset out of boundary. DATA_LENGTH=" & idata.Length.ToString & " OFFSET=" & offset.ToString)
        End If
        idata(offset) = (insert >> 24) And &HFF
        idata(offset + 1) = (insert >> 16) And &HFF
        idata(offset + 2) = (insert >> 8) And &HFF
        idata(offset + 3) = insert And &HFF
    End Sub

    Public Shared Function BytesEqual(A As Byte(), B As Byte()) As Boolean
        If (A.Length <> B.Length) Then
            Return False
        Else
            For i = 0 To A.Length - 1
                If A(i) <> B(i) Then
                    Return False
                End If
            Next
        End If
        Return True
    End Function

End Class
