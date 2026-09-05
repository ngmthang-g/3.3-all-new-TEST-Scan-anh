-- AUTO dồn đồ Thần Long PRO MAX v9.9 ĐẶC BIỆT
-- License security v2: 30-day keys, deep hashed PC fingerprint, automatic block on machine mismatch.

alter table public.licenses
  drop constraint if exists licenses_duration_days_check;

alter table public.licenses
  add constraint licenses_duration_days_check
  check (duration_days in (3,15,30,60,90,365));

alter table public.licenses
  add column if not exists legacy_machine_hash text,
  add column if not exists machine_profile jsonb not null default '{}'::jsonb,
  add column if not exists mismatch_machine_hash text,
  add column if not exists mismatch_machine_name text not null default '',
  add column if not exists mismatch_machine_profile jsonb not null default '{}'::jsonb,
  add column if not exists blocked_at timestamptz,
  add column if not exists blocked_reason text not null default '';

create or replace function public.license_validate_v2(
  p_key_hash text,
  p_device_hash text,
  p_legacy_device_hash text,
  p_machine_profile jsonb,
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
  v_profile jsonb := case when p_machine_profile is not null and jsonb_typeof(p_machine_profile) = 'object'
                          then p_machine_profile else '{}'::jsonb end;
  v_same_machine boolean := false;
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
        legacy_machine_hash = nullif(p_legacy_device_hash, ''),
        machine_profile = v_profile,
        machine_name = left(coalesce(p_machine_name,''),120),
        activated_at = v_now,
        expires_at = v_expires,
        blocked_at = null,
        blocked_reason = ''
    where key_hash = p_key_hash;
    v_license.machine_hash := p_device_hash;
    v_license.legacy_machine_hash := nullif(p_legacy_device_hash, '');
    v_license.machine_profile := v_profile;
    v_license.machine_name := left(coalesce(p_machine_name,''),120);
    v_license.activated_at := v_now;
    v_license.expires_at := v_expires;
  else
    v_same_machine :=
      v_license.machine_hash = p_device_hash
      or (coalesce(v_license.legacy_machine_hash,'') <> '' and v_license.legacy_machine_hash = p_device_hash)
      or (coalesce(p_legacy_device_hash,'') <> '' and v_license.machine_hash = p_legacy_device_hash)
      or (coalesce(p_legacy_device_hash,'') <> '' and coalesce(v_license.legacy_machine_hash,'') <> ''
          and v_license.legacy_machine_hash = p_legacy_device_hash);

    if not v_same_machine then
      update public.licenses
      set status = 'blocked',
          blocked_at = v_now,
          blocked_reason = 'machine_mismatch',
          mismatch_machine_hash = p_device_hash,
          mismatch_machine_name = left(coalesce(p_machine_name,''),120),
          mismatch_machine_profile = v_profile,
          last_seen_at = v_now,
          last_app_version = left(coalesce(p_app_version,''),32)
      where key_hash = p_key_hash;
      return jsonb_build_object(
        'ok', false,
        'code', 'machine_mismatch_blocked',
        'message', 'Phát hiện key được dùng trên PC khác. Key đã tự động bị chặn.'
      );
    end if;

    -- Nâng key đã bind bằng fingerprint v1 lên fingerprint v2 khi chính máy cũ mở bản mới.
    if v_license.machine_hash <> p_device_hash and coalesce(p_legacy_device_hash,'') <> ''
       and (v_license.machine_hash = p_legacy_device_hash
            or coalesce(v_license.legacy_machine_hash,'') = p_legacy_device_hash) then
      update public.licenses
      set legacy_machine_hash = coalesce(nullif(legacy_machine_hash,''), p_legacy_device_hash),
          machine_hash = p_device_hash,
          machine_profile = case when v_profile = '{}'::jsonb then machine_profile else v_profile end
      where key_hash = p_key_hash;
      v_license.legacy_machine_hash := coalesce(nullif(v_license.legacy_machine_hash,''), p_legacy_device_hash);
      v_license.machine_hash := p_device_hash;
      if v_profile <> '{}'::jsonb then v_license.machine_profile := v_profile; end if;
    elsif v_license.machine_profile = '{}'::jsonb and v_profile <> '{}'::jsonb then
      update public.licenses set machine_profile = v_profile where key_hash = p_key_hash;
      v_license.machine_profile := v_profile;
    end if;
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
    (p_key_hash, p_session_id, v_now, v_now, p_device_hash,
     left(coalesce(p_machine_name,''),120), left(coalesce(p_app_version,''),32))
  on conflict (key_hash, session_id) do update set
    last_heartbeat_at = excluded.last_heartbeat_at,
    machine_hash = excluded.machine_hash,
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

create or replace function public.license_heartbeat_v2(
  p_key_hash text,
  p_device_hash text,
  p_legacy_device_hash text,
  p_machine_profile jsonb,
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
  v_profile jsonb := case when p_machine_profile is not null and jsonb_typeof(p_machine_profile) = 'object'
                          then p_machine_profile else '{}'::jsonb end;
  v_same_machine boolean := false;
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

  v_same_machine :=
    v_license.machine_hash = p_device_hash
    or (coalesce(v_license.legacy_machine_hash,'') <> '' and v_license.legacy_machine_hash = p_device_hash)
    or (coalesce(p_legacy_device_hash,'') <> '' and v_license.machine_hash = p_legacy_device_hash)
    or (coalesce(p_legacy_device_hash,'') <> '' and coalesce(v_license.legacy_machine_hash,'') <> ''
        and v_license.legacy_machine_hash = p_legacy_device_hash);

  if not v_same_machine then
    update public.licenses
    set status = 'blocked',
        blocked_at = v_now,
        blocked_reason = 'machine_mismatch',
        mismatch_machine_hash = p_device_hash,
        mismatch_machine_name = left(coalesce(p_machine_name,''),120),
        mismatch_machine_profile = v_profile,
        last_seen_at = v_now,
        last_app_version = left(coalesce(p_app_version,''),32)
    where key_hash = p_key_hash;
    return jsonb_build_object(
      'ok', false,
      'code', 'machine_mismatch_blocked',
      'message', 'Phát hiện key được dùng trên PC khác. Key đã tự động bị chặn.'
    );
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
      machine_profile = case when machine_profile = '{}'::jsonb and v_profile <> '{}'::jsonb then v_profile else machine_profile end,
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

revoke all on function public.license_validate_v2(text,text,text,jsonb,text,text,text) from public, anon, authenticated;
revoke all on function public.license_heartbeat_v2(text,text,text,jsonb,text,text,text) from public, anon, authenticated;
grant execute on function public.license_validate_v2(text,text,text,jsonb,text,text,text) to service_role;
grant execute on function public.license_heartbeat_v2(text,text,text,jsonb,text,text,text) to service_role;
