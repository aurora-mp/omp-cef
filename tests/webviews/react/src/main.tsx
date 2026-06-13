import { createRoot } from 'react-dom/client';
import { AppProviders } from './providers/app-providers';
import App from './App';
import './index.css';

createRoot(document.getElementById('root')!).render(
    <AppProviders>
        <App />
    </AppProviders>,
);
