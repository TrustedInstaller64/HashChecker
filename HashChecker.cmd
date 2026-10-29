@rem ============================================================================
@rem HashChecker.cmd —— 项目最初始的旧版批处理实现（certutil 版）
@rem
@rem 说明：本脚本是 HashChecker 最早的原型：调用 Windows 自带的 certutil 对两个
@rem 文件夹逐文件计算哈希并写入临时日志，再对日志整体排序、用 fc 比较，输出
@rem differ.log。它已被 C++ 版（src/main.cpp 编译出的 hashchecker）取代，
@rem 仅作为历史存档保留，不参与构建，也不会被 build.sh / build.bat / CI 调用。
@rem
@rem 用法：在本目录下双击运行，按提示输入目标文件夹与参考文件夹路径。
@rem ============================================================================

@echo on
setlocal EnableDelayedExpansion

:: Created By TrustedInstaller64

set "hash1="
set "hash2="

set /p CopiedFile=请输入校对目标文件夹路径(请确保文件路径正确:):
if "%CopiedFile%"=="" (
	echo 检测到输入为空。
	goto :eof
)
if not "%CopiedFile:~-1%"=="\" set "CopiedFile=%CopiedFile%\"
set /p OriginalFile=请输入校对参考文件夹的路径(请确保文件路径正确:):
if "%OriginalFile%"=="" (
	echo 检测到输入为空。
	goto :eof
)
if not "%OriginalFile:~-1%"=="\" set "OriginalFile=%OriginalFile%\"

set /p HashClass=请输入校验类型(MD5/SHA1/SHA256, 将默认使用SHA256:):
if "%HashClass%"=="" (
	echo 检测到输入为空。
	echo 将默认使用SHA256
	set HashClass=SHA256
)

if exist CertUtilTemp rd /s /q CertUtilTemp
mkdir CertUtilTemp
attrib CertUtilTemp +h

for /r "%CopiedFile%" %%f in (*) do (
	set "rel=%%f"
	set "rel=!rel:%CopiedFile%=!"
	for /f "tokens=1" %%h in ('certutil -hashfile "%%f" %HashClass% ^| findstr /v "hash CertUtil"') do (
		echo !rel! ^| %%h >> ./CertUtilTemp/hash1.log
	)
)

for /r "%OriginalFile%" %%f in (*) do (
	set "rel=%%f"
	set "rel=!rel:%OriginalFile%=!"
	for /f "tokens=1" %%h in ('certutil -hashfile "%%f" %HashClass% ^| findstr /v "hash CertUtil"') do (
		echo !rel! ^| %%h >> ./CertUtilTemp/hash2.log
	)


)

sort ./CertUtilTemp/hash1.log /o ./CertUtilTemp/hash1_sorted.log
sort ./CertUtilTemp/hash2.log /o ./CertUtilTemp/hash2_sorted.log

for /f "tokens=1" %%a in ('certutil -hashfile "./CertUtilTemp/hash1_sorted.log" SHA256 ^| findstr /v "hash CertUtil"') do (
	set hash1=%%a
)
for /f "tokens=1" %%a in ('certutil -hashfile "./CertUtilTemp/hash2_sorted.log" SHA256 ^| findstr /v "hash CertUtil"') do (
	set hash2=%%a
)

if "%hash1%"=="%hash2%"  (
	echo CertUtil 返回文件差异为0
)
else (
	echo Certutil 返回文件存在差异
	fc /n ./CertUtilTemp/hash1_sorted.log ./CertUtilTemp/hash2_sorted.log >> ./CertUtilTemp/differ.log
)

set /p WhetherSaveLog=是否保存Certutil日志?(y/N)
if "%WhetherSaveLog%"=="" (
	echo 检测到输入为空, 程序默认保存日志于CertUtil Log目录下。
	attrib CertUtilTemp -h
	echo 日志已保存
	pause
)
else if /i "%WhetherSaveLog%"=="y" (
	attrib CertUtilTemp -h
	echo 日志已保存
	pause
)
else (
	rd /s /q CertUtilTemp
	echo 临时日志已删除.
	Pause
)

