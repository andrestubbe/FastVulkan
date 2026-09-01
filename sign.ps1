$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -match 'FastJava' } | Select-Object -First 1

if ($cert) {
    # Place self-signed certificate into CurrentUser\Root so Windows recognizes it as Trusted Root CA
    $rootStore = [System.Security.Cryptography.X509Certificates.X509Store]::new("Root", "CurrentUser")
    $rootStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    if (-not ($rootStore.Certificates | Where-Object { $_.Thumbprint -eq $cert.Thumbprint })) {
        $rootStore.Add($cert)
    }
    $rootStore.Close()

    # Place into CurrentUser\TrustedPublisher
    $pubStore = [System.Security.Cryptography.X509Certificates.X509Store]::new("TrustedPublisher", "CurrentUser")
    $pubStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    if (-not ($pubStore.Certificates | Where-Object { $_.Thumbprint -eq $cert.Thumbprint })) {
        $pubStore.Add($cert)
    }
    $pubStore.Close()
}

$dlls = @("release\fastvulkan.dll", "fastvulkan.dll")
foreach ($dll in $dlls) {
    if (Test-Path $dll) {
        $res = Set-AuthenticodeSignature -FilePath $dll -Certificate $cert -HashAlgorithm SHA256
        Write-Host "Signed $($dll) - Status: [$($res.Status)]"
    }
}
