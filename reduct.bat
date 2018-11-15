@echo off
for /r %%f in (*.mdl) do (
	echo %%f
	start /b /w ./autosimplifyformdl64.exe "%%f" "r1_" "%%~nf.mdl.r1" > "%%~nf.r1_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r1" "r2_" "%%~nf.mdl.r2" > "%%~nf.r2_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r2" "r3_" "%%~nf.mdl.r3" > "%%~nf.r3_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r3" "r4_" "%%~nf.mdl.r4" > "%%~nf.r4_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r4" "r5_" "%%~nf.mdl.r5" > "%%~nf.r5_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r5" "r6_" "%%~nf.mdl.r6" > "%%~nf.r6_list"
)