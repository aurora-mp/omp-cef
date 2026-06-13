import { ReactNode } from 'react';
import { CefProvider } from './cef-provider';

export function AppProviders({ children }: { children: ReactNode }) {
    return <CefProvider>{children}</CefProvider>;
}
