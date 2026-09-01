@echo off
setlocal

echo Signing FastVulkan native DLLs...

powershell -NoProfile -ExecutionPolicy Bypass -Command "$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -match 'FastJava' } | Select-Object -First 1; if (-not $cert) { Write-Host 'Creating FastJava Code Signing Certificate...' -ForegroundColor Yellow; $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=FastJava OpenSource, O=FastJava Community' -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddYears(5); } $dlls = Get-ChildItem -Path . -Recurse -Filter *.dll | Where-Object { $_.FullName -notmatch '\\(target|\.git)\\' }; foreach ($dll in $dlls) { Write-Host ('Signing ' + $dll.Name + ' ... ') -NoNewline; $res = Set-AuthenticodeSignature -FilePath $dll.FullName -Certificate $cert -HashAlgorithm SHA256; Write-Host ('[' + $res.Status + ']') -ForegroundColor Green; }"

echo DLL signing finished.
