# Made by Chat GPT, couldnt get it to work
# The idea was, that i could launch SC2 on windows
# and then connect to it from WSL
# The SC2 C++ API doesnt like it though, if it doesnt control
# the sc2 process itself
# i got it to connect though, but couldnt start a match
# maybe i will revisit this one day

# Change this to match your installation
$SC2Exe = "C:\Program Files (x86)\StarCraft II\Versions\Base97563\SC2_x64.exe"

$ListenAddress = "127.0.0.1"
$Port = 5000

if (!(Test-Path $SC2Exe)) {
    Write-Error "SC2 executable not found:"
    Write-Host "  $SC2Exe"
    exit 1
}

Write-Host "Starting StarCraft II..."

Start-Process -WorkingDirectory "C:\Program Files (x86)\StarCraft II\Support64" -FilePath $SC2Exe -ArgumentList @(
    "-listen", $ListenAddress,
    "-port", $Port,
    "-displayMode", "0"
)

Write-Host "Waiting for SC2 API server..."

while ($true) {
    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.Connect($ListenAddress, $Port)
        $client.Close()

        Write-Host ""
        Write-Host "SC2 is ready!"
        Write-Host "Connect your bot to $ListenAddress`:$Port"
        break
    }
    catch {
        Start-Sleep -Milliseconds 250
    }
}