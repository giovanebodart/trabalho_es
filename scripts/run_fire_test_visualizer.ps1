[CmdletBinding()]
param([switch]$Demo, [switch]$BuildOnly)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build\debug"
$executable = Join-Path $buildDirectory "fire_test_visualizer.exe"
$visualizerSource = Join-Path $projectRoot "examples\fire_test_visualizer.c"
$includeDirectory = Join-Path $projectRoot "include"
$sourceDirectory = Join-Path $projectRoot "src"
$compiler = (Get-Command gcc -ErrorAction Stop).Source
$gcSources = @(
    "gc.c", "allocator.c", "interval_tree.c", "marker.c",
    "old_pages.c", "register_roots.c", "roots.c", "stack_roots.c",
    "sweeper.c"
) | ForEach-Object { Join-Path $sourceDirectory $_ }

New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
$compilerArguments = @(
    "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-g3", "-O0", "-fno-omit-frame-pointer",
    "-I$includeDirectory", $visualizerSource
) + $gcSources + @("-o", $executable, "-lpsapi")

Write-Host "Compilando visualizador do fire_test em modo debug..."
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
