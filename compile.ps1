$files = Get-ChildItem -Path "./src" -File -Recurse | Select-Object -ExpandProperty FullName

gcc -DDEBUG_PRINT_AST -DDEBUG_PRINT_CHUNK -std=c99  .\main.c @files -I ./lib -o main