import { useCef } from '@/providers/cef-provider';
import EscapeMenuExample from './components/escape-menu-example';
import Login from './pages/login';

function App() {
    const { isReady } = useCef();

    return (
        <div className={!isReady ? 'devbg' : ''}>
            {isReady && (
                <>
                    <Login />
                    <EscapeMenuExample />
                </>
            )}
        </div>
    );
}

export default App;
