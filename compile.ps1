$files = Get-ChildItem -Path "./src" -File -Recurse | Select-Object -ExpandProperty FullName

gcc .\main.c @files -I ./lib -o main