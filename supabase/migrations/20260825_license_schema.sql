-- AUTO Thần Long đa tính năng Pro v3.4
-- Supabase license schema. Backend-only tables: browser/EXE never access them directly.

create table if not exists public.license_admins (
  user_id uuid primary key references auth.users(id) on delete cascade,
  enabled boolean not null default true,
  created_at timestamptz not null default now()
);

create table if not exists public.licenses (
  key_hash text primary key check (key_hash ~ '^[a-f0-9]{64}$'),
  key_text text not null unique,
  duration_days integer not null check (duration_days in (3,15,60,90,365)),
  status text not null default 'allowed' check (status in ('allowed','blocked')),
  note text not null default '' check (char_length(note) <= 240),
  machine_hash text,
  machine_name text not null default '',
  activated_at timestamptz,
  expires_at timestamptz,
  created_at timestamptz not null default now(),
  created_by_user uuid,
  created_by_email text not null default '',
  last_seen_at timestamptz,
  last_app_version text not null default '',
  launch_count bigint not null default 0,
  today_date date,
  today_active_seconds integer not null default 0
);

create index if not exists licenses_created_at_idx on public.licenses (created_at desc);

create table if not exists public.license_sessions (
  key_hash text not null references public.licenses(key_hash) on delete cascade,
  session_id text not null,
  started_at timestamptz not null default now(),
  last_heartbeat_at timestamptz not null default now(),
  machine_hash text not null,
  machine_name text not null default '',
  app_version text not null default '',
  primary key (key_hash, session_id)
);

create table if not exists public.license_daily (
  key_hash text not null references public.licenses(key_hash) on delete cascade,
  day date not null,
  first_seen_at timestamptz not null default now(),
  last_seen_at timestamptz not null default now(),
  launches integer not null default 0,
  active_seconds integer not null default 0,
  machine_name text not null default '',
  app_version text not null default '',
  primary key (key_hash, day)
);

alter table public.license_admins enable row level security;
alter table public.licenses enable row level security;
alter table public.license_sessions enable row level security;
alter table public.license_daily enable row level security;

-- No RLS policies are intentionally created. Public/authenticated clients have no direct table access.
revoke all on table public.license_admins from anon, authenticated;
revoke all on table public.licenses from anon, authenticated;
revoke all on table public.license_sessions from anon, authenticated;
revoke all on table public.license_daily from anon, authenticated;

create or replace function public.license_validate(
  p_key_hash text,
  p_device_hash text,
  p_machine_name text,
  p_session_id text,
  p_app_version text
) returns jsonb
language plpgsql
security definer
set search_path = public
as $$
declare
  v_license public.licenses%rowtype;
  v_now timestamptz := clock_timestamp();
  v_day date := (clock_timestamp() at time zone 'Asia/Ho_Chi_Minh')::date;
  v_new_session boolean := false;
  v_expires timestamptz;
begin
  select * into v_license
  from public.licenses
  where key_hash = p_key_hash
  for update;

  if not found then
    return jsonb_build_object('ok', false, 'code', 'not_found', 'message', 'Key không tồn tại.');
  end if;
  if v_license.status <> 'allowed' then
    return jsonb_build_object('ok', false, 'code', 'blocked', 'message', 'Key đã bị chặn.');
  end if;

  if coalesce(v_license.machine_hash, '') = '' then
    v_expires := v_now + make_interval(days => v_license.duration_days);
    update public.licenses
    set machine_hash = p_device_hash,
        machine_name = left(coalesce(p_machine_name,''),120),
        activated_at = v_now,
        expires_at = v_expires
    where key_hash = p_key_hash;
    v_license.machine_hash := p_device_hash;
    v_license.machine_name := left(coalesce(p_machine_name,''),120);
    v_license.activated_at := v_now;
    v_license.expires_at := v_expires;
  elsif v_license.machine_hash <> p_device_hash then
    return jsonb_build_object('ok', false, 'code', 'machine_mismatch', 'message', 'Key đã được kích hoạt trên máy khác.');
  end if;

  if v_license.expires_at is null or v_license.expires_at <= v_now then
    return jsonb_build_object('ok', false, 'code', 'expired', 'message', 'Key đã hết hạn.');
  end if;

  select not exists (
    select 1 from public.license_sessions
    where key_hash = p_key_hash and session_id = p_session_id
  ) into v_new_session;

  insert into public.license_sessions
    (key_hash, session_id, started_at, last_heartbeat_at, machine_hash, machine_name, app_version)
  values
    (p_key_hash, p_session_id, v_now, v_now, p_device_hash, left(coalesce(p_machine_name,''),120), left(coalesce(p_app_version,''),32))
  on conflict (key_hash, session_id) do update set
    last_heartbeat_at = excluded.last_heartbeat_at,
    machine_name = excluded.machine_name,
    app_version = excluded.app_version;

  insert into public.license_daily
    (key_hash, day, first_seen_at, last_seen_at, launches, active_seconds, machine_name, app_version)
  values
    (p_key_hash, v_day, v_now, v_now, case when v_new_session then 1 else 0 end, 0,
     left(coalesce(p_machine_name,''),120), left(coalesce(p_app_version,''),32))
  on conflict (key_hash, day) do update set
    last_seen_at = excluded.last_seen_at,
    launches = public.license_daily.launches + case when v_new_session then 1 else 0 end,
    machine_name = excluded.machine_name,
    app_version = excluded.app_version;

  update public.licenses
  set machine_name = coalesce(nullif(left(coalesce(p_machine_name,''),120),''), machine_name),
      last_seen_at = v_now,
      last_app_version = left(coalesce(p_app_version,''),32),
      launch_count = launch_count + case when v_new_session then 1 else 0 end,
      today_date = v_day,
      today_active_seconds = case when today_date = v_day then today_active_seconds else 0 end
  where key_hash = p_key_hash;

  return jsonb_build_object(
    'ok', true,
    'message', 'License hợp lệ.',
    'expiresAt', to_char(v_license.expires_at at time zone 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'),
    'remainingSeconds', greatest(0, floor(extract(epoch from (v_license.expires_at - v_now)))::bigint),
    'heartbeatSeconds', 300
  );
end;
$$;

create or replace function public.license_heartbeat(
  p_key_hash text,
  p_device_hash text,
  p_machine_name text,
  p_session_id text,
  p_app_version text
) returns jsonb
language plpgsql
security definer
set search_path = public
as $$
declare
  v_license public.licenses%rowtype;
  v_session public.license_sessions%rowtype;
  v_now timestamptz := clock_timestamp();
  v_day date := (clock_timestamp() at time zone 'Asia/Ho_Chi_Minh')::date;
  v_elapsed integer := 0;
begin
  select * into v_license
  from public.licenses
  where key_hash = p_key_hash
  for update;

  if not found then
    return jsonb_build_object('ok', false, 'code', 'not_found', 'message', 'Key không tồn tại.');
  end if;
  if v_license.status <> 'allowed' then
    return jsonb_build_object('ok', false, 'code', 'blocked', 'message', 'Key đã bị chặn.');
  end if;
  if coalesce(v_license.machine_hash,'') <> p_device_hash then
    return jsonb_build_object('ok', false, 'code', 'machine_mismatch', 'message', 'Key đã được kích hoạt trên máy khác.');
  end if;
  if v_license.expires_at is null or v_license.expires_at <= v_now then
    return jsonb_build_object('ok', false, 'code', 'expired', 'message', 'Key đã hết hạn.');
  end if;

  select * into v_session
  from public.license_sessions
  where key_hash = p_key_hash and session_id = p_session_id
  for update;

  if not found or v_session.machine_hash <> p_device_hash then
    return jsonb_build_object('ok', false, 'code', 'invalid_session', 'message', 'Phiên license không hợp lệ. Hãy mở lại tool.');
  end if;

  v_elapsed := greatest(0, least(600, floor(extract(epoch from (v_now - v_session.last_heartbeat_at)))::integer));

  update public.license_sessions
  set last_heartbeat_at = v_now,
      machine_name = left(coalesce(p_machine_name,''),120),
      app_version = left(coalesce(p_app_version,''),32)
  where key_hash = p_key_hash and session_id = p_session_id;

  insert into public.license_daily
    (key_hash, day, first_seen_at, last_seen_at, launches, active_seconds, machine_name, app_version)
  values
    (p_key_hash, v_day, v_now, v_now, 0, v_elapsed,
     left(coalesce(p_machine_name,''),120), left(coalesce(p_app_version,''),32))
  on conflict (key_hash, day) do update set
    last_seen_at = excluded.last_seen_at,
    active_seconds = public.license_daily.active_seconds + v_elapsed,
    machine_name = excluded.machine_name,
    app_version = excluded.app_version;

  update public.licenses
  set last_seen_at = v_now,
      machine_name = coalesce(nullif(left(coalesce(p_machine_name,''),120),''), machine_name),
      last_app_version = left(coalesce(p_app_version,''),32),
      today_date = v_day,
      today_active_seconds = case when today_date = v_day then today_active_seconds else 0 end + v_elapsed
  where key_hash = p_key_hash;

  return jsonb_build_object(
    'ok', true,
    'message', 'License hợp lệ.',
    'expiresAt', to_char(v_license.expires_at at time zone 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'),
    'remainingSeconds', greatest(0, floor(extract(epoch from (v_license.expires_at - v_now)))::bigint)
  );
end;
$$;

revoke all on function public.license_validate(text,text,text,text,text) from public, anon, authenticated;
revoke all on function public.license_heartbeat(text,text,text,text,text) from public, anon, authenticated;
grant execute on function public.license_validate(text,text,text,text,text) to service_role;
grant execute on function public.license_heartbeat(text,text,text,text,text) to service_role;
