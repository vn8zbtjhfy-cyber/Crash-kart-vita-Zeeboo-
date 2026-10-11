# Optional one-command publisher for the clean source ZIP on Windows.
# Requires git and already-authorized access to the private GitHub repository.
# Never uploads ROMs or any file from the full development ZIP.
param(
    [Parameter(Mandatory=$true)][string]$Zip,
    [string]$Repository = "https://github.com/vn8zbtjhfy-cyber/Crash-kart-vita-Zeeboo-.git"
)
$ErrorActionPreference="Stop"
if (!(Get-Command git -ErrorAction SilentlyContinue)) { throw "Instala Git for Windows antes de continuar." }
$source=(Resolve-Path $Zip).Path
if (!(Test-Path $source -PathType Leaf)) { throw "No existe el ZIP de fuentes limpias." }
$parent=Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString("N"))
$extract=Join-Path $parent "fuentes"
$clone=Join-Path $parent "repo"
New-Item -ItemType Directory -Path $extract -Force | Out-Null
try {
    Expand-Archive -LiteralPath $source -DestinationPath $extract -Force
    # Stop before upload if ROM/asset files are mixed into the clean archive.
    $forbidden=Get-ChildItem -LiteralPath $extract -File -Recurse -Force | Where-Object {
        $_.Name -match '^(data\.vfs|cnk2\.mod|cnk2\.sig)$' -or
        $_.Extension -match '^\.(vpk|self|elf|velf|mod|sig|mif|wav|ogg|mp3|png|jpg|jpeg|zip)$'
    }
    if ($forbidden) { throw "ZIP inseguro: contiene datos o binarios originales. Publicacion cancelada." }
    if (!(Test-Path (Join-Path $extract "CMakeLists.txt"))) { throw "No hay CMakeLists.txt en la raiz del ZIP." }
    if (!(Test-Path (Join-Path $extract "src/vita_main.c"))) { throw "No contiene src/vita_main.c." }
    git clone $Repository $clone
    if ($LASTEXITCODE -ne 0) { throw "Git no pudo clonar el repositorio. Revisa autorizacion." }
    Get-ChildItem -LiteralPath $extract -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $clone -Recurse -Force
    }
    Push-Location $clone
    try {
        git add -A
        if ($LASTEXITCODE -ne 0) { throw "Fallo git add." }
        $staged=git diff --cached --name-only
        if ($staged) {
            git commit -m "Importar fuentes limpias del avance 107 para el enlace VitaSDK"
            if ($LASTEXITCODE -ne 0) { throw "Fallo git commit. Configura nombre y correo de Git." }
            git push origin main
            if ($LASTEXITCODE -ne 0) { throw "Fallo git push. Los archivos no fueron publicados." }
        } else { Write-Host "Sin cambios por publicar." }
        Write-Host "Publicacion completada. Revisa GitHub Actions: VitaSDK diagnostic ELF."
    } finally { Pop-Location }
} finally {
    Remove-Item -LiteralPath $parent -Force -Recurse -ErrorAction SilentlyContinue
}
