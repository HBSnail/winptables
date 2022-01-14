<System.ComponentModel.RunInstaller(True)> Partial Class ProjectInstaller
    Inherits System.Configuration.Install.Installer

    'Installer overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Component Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Component Designer
    'It can be modified using the Component Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()>
    Private Sub InitializeComponent()
        Me.WinptablesProcessInstaller = New System.ServiceProcess.ServiceProcessInstaller()
        Me.WinptablesServiceInstaller = New System.ServiceProcess.ServiceInstaller()
        '
        'WinptablesProcessInstaller
        '
        Me.WinptablesProcessInstaller.Account = System.ServiceProcess.ServiceAccount.LocalSystem
        Me.WinptablesProcessInstaller.Password = Nothing
        Me.WinptablesProcessInstaller.Username = Nothing
        '
        'WinptablesServiceInstaller
        '
        Me.WinptablesServiceInstaller.Description = "A service to control the Winptables NDIS Driver. Disable this service may cause n" &
    "etwork not available."
        Me.WinptablesServiceInstaller.DisplayName = "Winptables Service"
        Me.WinptablesServiceInstaller.ServiceName = "Winptables Service"
        Me.WinptablesServiceInstaller.StartType = System.ServiceProcess.ServiceStartMode.Automatic
        '
        'ProjectInstaller
        '
        Me.Installers.AddRange(New System.Configuration.Install.Installer() {Me.WinptablesProcessInstaller, Me.WinptablesServiceInstaller})

    End Sub
    Public WithEvents WinptablesProcessInstaller As ServiceProcess.ServiceProcessInstaller
    Public WithEvents WinptablesServiceInstaller As ServiceProcess.ServiceInstaller
End Class
