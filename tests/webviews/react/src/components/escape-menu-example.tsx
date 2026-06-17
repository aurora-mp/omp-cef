import { useEffect, useState } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { useCef } from '@/providers/cef-provider';

export default function EscapeMenuExample() {
    const { on, off } = useCef();
    const [visible, setVisible] = useState(false);

    useEffect(() => {
        const handleEscapeMenu = (nextVisible: boolean) => {
            setVisible(Boolean(nextVisible));
        };

        on('cef:escape_menu', handleEscapeMenu);

        return () => {
            off('cef:escape_menu', handleEscapeMenu);
        };
    }, [on, off]);

    if (!visible) {
        return null;
    }

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70">
            <Card className="w-96 border-white/10 bg-slate-950/95 text-white shadow-2xl">
                <CardHeader>
                    <CardTitle>Custom ESC Menu</CardTitle>
                </CardHeader>

                <CardContent className="space-y-3 text-sm text-slate-300">
                    <p>The native GTA pause menu is blocked.</p>
                    <p>This overlay is displayed from the JS event:</p>
                    <code className="block rounded bg-black/40 px-3 py-2 text-xs text-primary">cef:escape_menu(boolean)</code>
                    <p className="text-xs text-slate-400">Press ESC again to hide this example menu.</p>
                </CardContent>
            </Card>
        </div>
    );
}
