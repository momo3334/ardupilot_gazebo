@echo off
REM Point Gazebo at the plugins built by `pixi run build` and at the models and
REM worlds in this repository.
REM
REM This lives in a script rather than in [activation.env] because pixi hands
REM the values in that table to the platform shell verbatim. On Windows that
REM shell is cmd.exe, which never gets a chance to expand %PIXI_PROJECT_ROOT%,
REM so the paths would end up literal. Activation scripts are executed, so the
REM expansion happens normally here.
REM
REM The models directory must come LAST in GZ_SIM_RESOURCE_PATH. Gazebo splits
REM these lists on ':' even on Windows, which chops every "C:\..." entry in
REM two, and only the final fragment survives to resolve model:// URIs. A
REM drive-less absolute path still resolves against the current drive, so
REM putting models last is what makes the includes load.

if defined GZ_SIM_SYSTEM_PLUGIN_PATH (
    set "GZ_SIM_SYSTEM_PLUGIN_PATH=%PIXI_PROJECT_ROOT%\build-win64;%GZ_SIM_SYSTEM_PLUGIN_PATH%"
) else (
    set "GZ_SIM_SYSTEM_PLUGIN_PATH=%PIXI_PROJECT_ROOT%\build-win64"
)

if defined GZ_SIM_RESOURCE_PATH (
    set "GZ_SIM_RESOURCE_PATH=%GZ_SIM_RESOURCE_PATH%;%PIXI_PROJECT_ROOT%\worlds;%PIXI_PROJECT_ROOT%\models"
) else (
    set "GZ_SIM_RESOURCE_PATH=%PIXI_PROJECT_ROOT%\worlds;%PIXI_PROJECT_ROOT%\models"
)

REM Two COLLADA meshes reference their textures relative to their own
REM directory, as "../materials/textures/...". Gazebo fails to resolve those
REM against the mesh location on Windows and falls back to searching
REM GZ_SIM_RESOURCE_PATH, where they are not found. GZ_FILE_PATH is a separate
REM lookup that does honour every entry, so listing the mesh directories here
REM makes the textures resolve. Add an entry for any new model whose mesh
REM refers to textures this way.
if defined GZ_FILE_PATH (
    set "GZ_FILE_PATH=%GZ_FILE_PATH%;%PIXI_PROJECT_ROOT%\models\runway\meshes;%PIXI_PROJECT_ROOT%\models\zephyr\meshes"
) else (
    set "GZ_FILE_PATH=%PIXI_PROJECT_ROOT%\models\runway\meshes;%PIXI_PROJECT_ROOT%\models\zephyr\meshes"
)
