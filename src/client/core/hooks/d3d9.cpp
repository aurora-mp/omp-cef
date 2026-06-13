#include "d3d9.hpp"

// GTA SA 1.0 addresses
volatile IDirect3D9*& gGameDirect = *reinterpret_cast<volatile IDirect3D9**>(0xC97C20);
volatile IDirect3DDevice9*& gGameDevice = *reinterpret_cast<volatile IDirect3DDevice9**>(0xC97C28);