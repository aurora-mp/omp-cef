import { useCef } from '@/providers/cef-provider';
import Login from './pages/login';

function App() {
    const { isReady } = useCef();

    return <div className={!isReady ? 'devbg' : ''}>{isReady && <Login />}</div>;
}

export default App;
