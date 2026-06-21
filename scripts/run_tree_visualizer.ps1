[CmdletBinding()]
param([switch]$Demo, [switch]$BuildOnly)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build\debug"
$executable = Join-Path $buildDirectory "interval_tree_visualizer.exe"
$visualizerSource = Join-Path $projectRoot "examples\interval_tree_visualizer.c"
$treeSource = Join-Path $projectRoot "src\interval_tree.c"
$includeDirectory = Join-Path $projectRoot "include"
$compiler = (Get-Command gcc -ErrorAction Stop).Source

New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
$compilerArguments = @(
    "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-g3", "-O0", "-fno-omit-frame-pointer",
    "-I$includeDirectory", $visualizerSource, $treeSource,
    "-o", $executable
)

Write-Host "Compilando visualizador em modo debug..."
& $compiler @compilerArguments
if ($LASTEXITCODE -ne 0) {
    throw "A compilacao falhou com codigo $LASTEXITCODE."
}

Write-Host "Build concluido sem warnings: $executable"
if ($BuildOnly) {
    return
}

if ($Demo) {
    & $executable "--demo"
} else {
    & $executable
}
if ($LASTEXITCODE -ne 0) {
    throw "O visualizador terminou com codigo $LASTEXITCODE."
}
