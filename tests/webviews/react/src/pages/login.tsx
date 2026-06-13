import { useEffect, useState } from 'react';
import { useForm } from 'react-hook-form';
import { zodResolver } from '@hookform/resolvers/zod';
import * as z from 'zod';
import { Card, CardContent, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Form, FormControl, FormField, FormItem, FormLabel, FormMessage } from '@/components/ui/form';
import { Input } from '@/components/ui/input';
import { IAccount } from '@/types';
import { useCef } from '@/providers/cef-provider';

const formSchema = z.object({
    password: z.string().min(6, 'The password must contain a minimum of 6 characters.'),
});

export default function Login() {
    const { emit, on, off } = useCef();
    const [account, setAccount] = useState<IAccount | null>(null);
    const [submitted, setSubmitted] = useState(false);
    const [error, setError] = useState('');

    const form = useForm<z.infer<typeof formSchema>>({
        resolver: zodResolver(formSchema),
        defaultValues: {
            password: '',
        },
    });

    useEffect(() => {
        const handleAccountInfo = (name: string) => {
            try {
                setAccount({
                    name,
                });
            } catch {}
        };

        on('account:info', handleAccountInfo);

        return () => {
            off('account:info', handleAccountInfo);
        };
    }, [emit, on, off]);

    const onSubmit = (values: z.infer<typeof formSchema>) => {
        setSubmitted(true);
        setError('');

        emit('account:login', values.password);
    };

    return (
        <div className="h-screen flex flex-col justify-center items-center">
            <Card className="border-0 min-w-92.5 max-w-92.5">
                <CardHeader>
                    <CardTitle className="text-white">Login</CardTitle>
                </CardHeader>

                <CardContent>
                    <p className="text-sm">
                        Hello <span className="text-primary font-medium">{account?.name}</span>,
                    </p>

                    <p className="text-sm pt-3">Please enter your password:</p>

                    <Form {...form}>
                        <form onSubmit={form.handleSubmit(onSubmit)}>
                            <FormField
                                control={form.control}
                                name="password"
                                render={({ field }) => (
                                    <FormItem className="mt-5">
                                        <FormLabel>Password</FormLabel>
                                        <FormControl>
                                            <Input type="password" autoComplete="off" {...field} />
                                        </FormControl>
                                        <FormMessage />
                                    </FormItem>
                                )}
                            />

                            <div className="text-right">
                                <Button type="submit" className="mt-5" disabled={submitted}>
                                    Login
                                </Button>
                            </div>
                        </form>
                    </Form>
                </CardContent>

                <CardFooter className="justify-center">{error && <div className="text-red-500 text-sm">{error}</div>}</CardFooter>
            </Card>
        </div>
    );
}
