/// <reference types="vite/client" />

interface CefScreenFrame {
    data: ArrayBuffer;
    width: number;
    height: number;
    sequence: number;
    timestamp: number;
    format: 'RGBA';
}

interface CefAPI {
    emit: (event: string, ...args: any[]) => void;
    on: (event: string, callback: (...args: any[]) => void) => void;
    off: (event: string, callback: (...args: any[]) => void) => void;
    screen: {
        start: (
            callback: (frame: CefScreenFrame) => void,
            width?: number,
            height?: number,
            fps?: number
        ) => boolean;
        stop: () => boolean;
    };
}

interface Window {
    cef: CefAPI;
}

declare const cef: CefAPI;
