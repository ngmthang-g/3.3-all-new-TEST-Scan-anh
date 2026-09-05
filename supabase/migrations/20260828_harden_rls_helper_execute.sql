-- Keep the legacy maintenance helper out of exposed anon/authenticated RPC access.
revoke all on function public.rls_auto_enable() from public, anon, authenticated;
grant execute on function public.rls_auto_enable() to service_role;
