/// <reference types="vite/client" />

interface CefAPI {
    emit: (event: string, ...args: any[]) => void;
    on: (event: string, callback: (...args: any[]) => void) => void;
    off: (event: string, callback: (...args: any[]) => void) => void;
}

interface Window {
    cef: CefAPI;
}

declare const cef: CefAPI;
