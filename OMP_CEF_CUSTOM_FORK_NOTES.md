# NSY-Cef Custom Fork Architecture Notes

**¡ATENCIÓN A FUTUROS MODELOS / ASISTENTES DE IA!**
Este repositorio es un **fork altamente personalizado** del proyecto original `aurora-mp/omp-cef`. Tiene modificaciones arquitectónicas críticas diseñadas específicamente para optimizar el rendimiento y evitar problemas de red/renderizado en el entorno de producción de NSY. 

**BAJO NINGUNA CIRCUNSTANCIA debes realizar un `git merge` directo o sobrescribir los siguientes sistemas con código *upstream*:**

### 1. Sistema de Renderizado de Texturas (`PendingPaint`)
- **Ubicación:** `src/client/core/browser/manager.cpp` y `manager.hpp`
- **Por qué no tocarlo:** El ciclo de renderizado original de CEF (`OnPaint`) causaba tirones (lag) en el hilo principal del juego al actualizar texturas. En este fork, se desarrolló un sistema de colas en CPU llamado `PendingPaint`.
- **Funcionamiento local:** 
  - `OnPaint` (CEF UI Thread) simplemente copia los píxeles en memoria RAM y marca el buffer como "sucio".
  - `RenderAll` (D3D Render Thread) es el encargado exclusivo de hacer el volcado a la VRAM (`view.UpdateTexture()`).
  - También incluye protección contra "ghosting" durante navegaciones SPA limpiando explícitamente los buffers.

### 2. Reconexión y Red (KCP Handshake)
- **Ubicación:** `src/client/core/app.cpp` y `src/server/common/plugin.cpp`
- **Por qué no tocarlo:** El usuario ha implementado un sistema robusto de auto-reconexión cuando el servidor de SA-MP se reinicia.
- **Funcionamiento local:**
  - El cliente detecta la desconexión (`!netGame->IsConnected()`) y limpia sus estados y sockets KCP mediante un `ResetSession()` agresivo.
  - El servidor fue parcheado previamente para evitar *deadlocks* en `HandleKcpInput`, procesando los paquetes y despachando eventos *fuera* del `mutex` principal del KCP.
  - **Nota (Junio 2026):** Se ha integrado adicionalmente la lógica de reconexión del lado del servidor del upstream (`52c118c`), lo que permite que el servidor destruya sesiones "zombie" y acepte nuevas conexiones bajo el mismo ID de jugador si el puerto cambia.

### Funciones Portadas Exitosamente desde Upstream (Junio 2026)
Se extrajeron e implementaron manualmente las siguientes características para mantener compatibilidad con las novedades, *sin romper la arquitectura local*:
- Corrección de bugs en texturas 3D (`WorldObject3D`) cuando un navegador está oculto.
- Soporte para **Menú de Escape Personalizado** (`CEF_SetEscapeMenuMode`).
- Soporte para **Scoreboard Personalizado** (`CEF_SetPlayerListMode`).
- Capacidad de recargar y navegar URLs dinámicamente (`CEF_LoadUrl`).

---
**Instrucción Final:** Si se solicita actualizar este repositorio con novedades de `aurora-mp`, inspecciona los commits del *upstream* y aplica las características (features) individualmente usando el sistema de parcheo manual. **No uses herramientas de auto-fusión (merge).**
