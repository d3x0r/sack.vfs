

echo LIBRARY SQLITE3 > nw.def
echo EXPORTS >> nw.def
dumpbin /exports nw.dll >zz
for /f "skip=19 tokens=4" %%A in ('dumpbin /exports nw.dll') do @echo %%A >> nw.def
lib /def:nw.def /out:nw.lib /machine:x64
