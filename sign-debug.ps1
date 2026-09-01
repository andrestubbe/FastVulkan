$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -match 'FastJava' } | Select-Object -First 1

if (-not $cert) {
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=FastJava Community, O=FastJava" -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddYears(5)
    # Also trust in Root / TrustedPublisher so Windows SmartScreen & Defender don't warn
    $store = Get-Item Cert:\CurrentUser\TrustedPublisher
    $store.Open("ReadWrite")
    $store.Add($cert)
    $store.Close()
}

$sig = Set-AuthenticodeSignature -FilePath "release\fastvulkan.dll" -Certificate $cert -HashAlgorithm SHA256 -TimestampServer "http://timestamp.digicert.com"
Write-Host "Status: " $sig.Status
Write-Host "StatusMessage: " $sig.StatusMessage

Copy-Item "release\fastvulkan.dll" "." -Force
