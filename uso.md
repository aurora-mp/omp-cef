# Guía de Compilación (uso.md)

Esta guía detalla los pasos necesarios para compilar el plugin CEF (tanto el componente nativo de open.mp como el plugin de SA-MP) desde el código fuente.

## 1. Requisitos Previos

Antes de comenzar, asegúrate de tener instalado lo siguiente:

*   **Visual Studio 2022**: Con la carga de trabajo "Desarrollo para el escritorio con C++" instalada.
*   **CMake**: Versión 3.21 o superior.
*   **vcpkg**: El gestor de paquetes de C++. Debe estar ubicado en una ruta accesible (ej: `C:\vcpkg` o en la carpeta superior de este proyecto).
*   **Git**: Para clonar submódulos si es necesario.

---

## 2. Configuración de Dependencias (vcpkg)

El proyecto utiliza **vcpkg** en modo manifiesto (`vcpkg.json`) para gestionar bibliotecas como `libsodium`, `nlohmann-json`, `miniz`, etc.

### Triplet de Destino
Para que el plugin sea compatible con servidores de 32 bits y no requiera DLLs externas de runtime, utilizamos el triplet:
`x86-windows-static`

Si no lo tienes instalado, vcpkg lo descargará automáticamente durante la configuración de CMake.

---

## 3. Proceso de Compilación

### Opción A: Usando CMake Presets (Recomendado)

El proyecto incluye `CMakePresets.json` que predefine las configuraciones más comunes.

1.  **Abrir terminal** en la raíz del proyecto (`omp-cef`).
2.  **Configurar el proyecto** (elige una opción):
    *   Para open.mp (Componente):
        ```powershell
        cmake --preset omp-win-x86-debug
        ```
    *   Para SA-MP (Plugin Legacy):
        ```powershell
        cmake --preset samp-win-x86-debug
        ```
    *   Para el Cliente:
        ```powershell
        cmake --preset client-win-x86-debug
        ```
3.  **Compilar**:
    ```powershell
    cmake --build build/<nombre-del-preset> --config Release
    ```

### Opción B: Comandos Manuales (Línea de comandos)

Si prefieres no usar presets, puedes usar estos comandos estándar:

```powershell
# Crear directorio de construcción
mkdir build-release
cd build-release

# Configurar con el toolchain de vcpkg y triplet estático
cmake .. -A Win32 `
    -DCMAKE_TOOLCHAIN_FILE="Ruta/A/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x86-windows-static `
    -DBUILD_SERVER_OMP=ON `
    -DBUILD_SERVER_SAMP=ON `
    -DBUILD_CLIENT=OFF

# Compilar en modo Release
cmake --build . --config Release
```

---

## 4. Resultados de Compilación

Una vez finalizado el proceso, encontrarás los archivos en las siguientes rutas dentro de tu carpeta de construcción (ej: `build-release`):

*   **Componente open.mp**: `src/server/omp/Release/Cef.dll`
*   **Plugin SA-MP**: `src/server/samp/Release/cef.dll`
*   **Cliente CEF**: `src/client/core/Release/client.dll`
*   **Cargador ASI**: `src/client/loader/Release/cef.asi`
*   **Renderizador**: `src/client/renderer/Release/renderer.exe`

---

## 5. Despliegue en el Servidor (NSY-OMP)

### Para open.mp (Recomendado)
1.  Copia `Cef.dll` a la carpeta `NSY-OMP/components/`.
2.  Asegúrate de que **no** esté `cef` en la lista de `legacy_plugins` en tu `config.json`.
3.  Reinicia el servidor.

### Para SA-MP / Legacy
1.  Copia `cef.dll` a la carpeta `NSY-OMP/plugins/`.
2.  Añade `cef` a la lista de `plugins` (SAMP) o `legacy_plugins` (open.mp).
3.  Reinicia el servidor.

---

## 6. Solución de Problemas Comunes

### Error: "vcpkg-running.lock"
Si una compilación se interrumpe, vcpkg puede dejar un archivo de bloqueo. Elimínalo manualmente en:
`build-release/vcpkg_installed/vcpkg/vcpkg-running.lock`

### Error: "CEF download timeout"
La primera vez que compiles el cliente, CMake descargará los binarios de CEF (~800MB). Esto puede tardar varios minutos dependiendo de tu conexión. Si falla, intenta ejecutar la configuración de nuevo.

### Error: Mismatch de Arquitectura
Asegúrate de compilar siempre para **x86** (32 bits), ya que open.mp y SA-MP son procesos de 32 bits. Usar `-A Win32` en CMake es obligatorio.
