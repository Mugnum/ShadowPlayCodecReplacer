# Self-elevate.
$currentIdentity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$currentPrincipal = [Security.Principal.WindowsPrincipal]::new($currentIdentity)
$adminRole        = [Security.Principal.WindowsBuiltInRole]::Administrator

if (-not $currentPrincipal.IsInRole($adminRole)) {
    Start-Process pwsh.exe -Verb RunAs -ArgumentList @(
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', "`"$PSCommandPath`""
    )
    exit
}

Get-Process -Name nvcontainer -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction Continue