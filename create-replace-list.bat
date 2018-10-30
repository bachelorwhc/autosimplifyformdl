@echo off
for /r %%f in (*.mdl) do (
	echo %%f
	start /b ./autosimplifyformdl64.exe "%%f" "%%~nf_reduct_" > "%%~nf.replace_list"
)