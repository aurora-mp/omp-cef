import { createContext, useContext, ReactNode, useCallback, useMemo } from 'react';

type CefCallback = (...args: any[]) => void;

interface CefContextType {
    isReady: boolean;
    emit: (event: string, ...args: any[]) => void;
    on: (event: string, callback: CefCallback) => void;
    off: (event: string, callback: CefCallback) => void;
}

const CefContext = createContext<CefContextType | null>(null);

export function CefProvider({ children }: { children: ReactNode }) {
    const isReady = 'cef' in window;

    const emit = useCallback(
        (event: string, ...args: any[]) => {
            if (!isReady) return;

            cef.emit(event, ...args);
        },
        [isReady],
    );

    const on = useCallback(
        (event: string, callback: CefCallback) => {
            if (!isReady) return;

            cef.on(event, callback);
        },
        [isReady],
    );

    const off = useCallback(
        (event: string, callback: CefCallback) => {
            if (!isReady) return;

            cef.off(event, callback);
        },
        [isReady],
    );

    const value = useMemo(() => {
        return { isReady, emit, on, off };
    }, [isReady, emit, on, off]);

    return <CefContext.Provider value={value}>{children}</CefContext.Provider>;
}

export function useCef() {
    const context = useContext(CefContext);
    if (!context) {
        throw new Error('useCef must be used within CefProvider');
    }

    return context;
}
