@echo off
for /r %%f in (*.mdl) do (
	echo %%f
	start /b /w ./autosimplifyformdl64.exe "%%f" "%%~nf_reduct_" "%%~nf.mdl.r" > "%%~nf.r_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.r" "%%~nf_rreduct_" "%%~nf.mdl.rr" > "%%~nf.rr_list"
	start /b /w ./autosimplifyformdl64.exe "%%~nf.mdl.rr" "%%~nf_rrreduct_" "%%~nf.mdl.rrr" > "%%~nf.rrr_list"
)