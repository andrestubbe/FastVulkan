$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -match 'FastJava' } | Select-Object -First 1
if (-not $cert) {
    Write-Host "Creating FastJava Code Signing Certificate..."
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=FastJava OpenSource, O=FastJava Community" -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddYears(5)
}

$cerPath = "$env:TEMP\fastjava_codesign.cer"
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Import-Certificate -CertStoreLocation Cert:\CurrentUser\Root -FilePath $cerPath | Out-Null
Import-Certificate -CertStoreLocation Cert:\CurrentUser\TrustedPublisher -FilePath $cerPath | Out-Null

Write-Host "FastJava Certificate trusted in CurrentUser\Root and TrustedPublisher."

$dlls = @(
    "release\fastvulkan.dll",
    "$env:USERPROFILE\.fastcore\native\fastvulkan\fastvulkan.dll",
    "src\main\resources\native\fastvulkan.dll"
)

foreach ($dll in $dlls) {
    if (Test-Path $dll) {
        $sig = Set-AuthenticodeSignature -FilePath $dll -Certificate $cert -HashAlgorithm SHA256
        Write-Host "Signed $($dll): $($sig.Status)"
        Unblock-File $dll
    }
}
