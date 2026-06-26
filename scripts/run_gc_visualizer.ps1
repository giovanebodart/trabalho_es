[CmdletBinding()]
param([switch]$Demo, [switch]$BuildOnly)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build\debug"
$executable = Join-Path $build "gc_visualizer.exe"
$include = Join-Path $root "include"
$source = Join-Path $root "src"
$compiler = (Get-Command gcc -ErrorAction Stop).Source
$gcSources = @(
    "gc.c", "allocator.c", "interval_tree.c", "marker.c",
    "old_pages.c", "register_roots.c", "roots.c", "stack_roots.c",
    "sweeper.c"
) | ForEach-Object { Join-Path $source $_ }
New-Item -ItemType Directory -Path $build -Force | Out-Null
$arguments = @(
    "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-g3", "-O0", "-fno-omit-frame-pointer", "-I$include", "-I$source",
    (Join-Path $root "examples\gc_visualizer.c")
) + $gcSources + @("-o", $executable, "-lpsapi")
Write-Host "Compilando visualizador do coletor em modo debug..."
& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "A compilacao falhou: $LASTEXITCODE." }
Write-Host "Build concluido sem warnings: $executable"
if ($BuildOnly) { return }
if ($Demo) { & $executable "--demo" } else { & $executable }
if ($LASTEXITCODE -ne 0) {
    throw "O visualizador terminou com codigo $LASTEXITCODE."
}
